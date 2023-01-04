#include "build.hpp"
#include "fallback.hpp"
#include "common/utils.hpp"
#include "common/win32/dwrite.hpp"
#include "common/win32/wincodec.hpp"
#include "common/win32/direct2d.hpp"

#include <Btk/context.hpp>
#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <wincodec.h>
#include <dwrite.h>
#include <variant>
#include <d2d1.h>
#include <limits>
#include <stack>
#include <wrl.h>
#include <map>

// D2D1 Extra
#include <d2d1_1.h>
#include <d2d1effects.h>

// DXGI / D3D11
#include <d3d11.h>
#include <dxgi.h>

#if defined(_MSC_VER)
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#endif

#undef min
#undef max

using Microsoft::WRL::ComPtr;

#if !defined(NDEBUG)
#define D2D_WARN(...) printf("D2D: " __VA_ARGS__)
#else
#define D2D_WARN(...)
#endif

// TODO List:
// 1. Add support for state saving/restoring.
// 2. Add support for copy texture to render target.

namespace {
    using namespace BTK_NAMESPACE;

    static_assert(sizeof(FMatrix) == sizeof(D2D1::Matrix3x2F));

    auto bounds_from_circle(float x, float y, float r) -> D2D1_RECT_F {
        return D2D1::RectF(
            x - r,
            y - r,
            x + r,
            y + r
        );
    }
    auto bounds_from_ellipse(float x, float y, float xr, float yr) -> D2D1_RECT_F {
        return D2D1::RectF(
            x - xr,
            y - yr,
            x + xr,
            y + yr
        );
    }
    auto bounds_from_line(float x1, float y1, float x2, float y2) -> D2D1_RECT_F {
        return D2D1::RectF(
            std::min(x1, x2),
            std::min(y1, y2),
            std::max(x1, x2),
            std::max(y1, y2)
        );
    }

    auto btkfmt_to_d2d(PixFormat b) -> D2D1_PIXEL_FORMAT {
        switch (b) {
            case PixFormat::RGBA32 : {
                return D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
            }
            case PixFormat::Gray8 : {
                return D2D1::PixelFormat(DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT);
            }
            default : {
                return D2D1::PixelFormat();
            }
        }
    }
    auto btkmat_to_d2d(const FMatrix &mat) -> D2D1::Matrix3x2F {
        return reinterpret_cast<const D2D1::Matrix3x2F&>(mat);
    }    
    auto d2dmat_to_btk(const D2D1::Matrix3x2F &mat) -> FMatrix {
        return reinterpret_cast<const FMatrix&>(mat);
    }

    auto layout_translate_by_align(IDWriteTextLayout *layout, Btk::Alignment align, float *x, float *y) {
        // Translate by align to left/top
        HRESULT hr;
        DWRITE_TEXT_METRICS metrics;
        hr = layout->GetMetrics(&metrics);
        assert(SUCCEEDED(hr));

        if ((align & Alignment::Right) == Alignment::Right) {
            *x -= metrics.width;
        }
        if ((align & Alignment::Bottom) == Alignment::Bottom) {
            *y -= metrics.height;
        }
        if ((align & Alignment::Center) == Alignment::Center) {
            *x -= metrics.width / 2;
        }
        if ((align & Alignment::Middle) == Alignment::Middle) {
            *y -= metrics.height / 2;
        }
        if ((align & Alignment::Baseline) == Alignment::Baseline) {
            // Get layout baseline
            DWRITE_LINE_METRICS line_metrics;
            UINT line_count = 0;
            hr = layout->GetLineMetrics(&line_metrics, 1, &line_count);
            assert(SUCCEEDED(hr));

            *y -= line_metrics.baseline;
        }

        return D2D1::RectF(*x, *y, *x + metrics.width, *y + metrics.height);
    }
    HDC d2d_lock_bitmap(ID2D1Bitmap *bitmap) {
        // Try cast to ID2D1Bitmap1
        ComPtr<IDXGISurface1> surface1;
        ComPtr<IDXGISurface> surface;
        ComPtr<ID2D1Bitmap1> bitmap1;

        if (FAILED(bitmap->QueryInterface(bitmap1.GetAddressOf()))) {
            return nullptr;
        }
        if (FAILED(bitmap1->GetSurface(surface.GetAddressOf()))) {
            return nullptr;
        }
        if (FAILED(surface.As(&surface1))) {
            return nullptr;
        }

        // Get DC
        HDC dc;
        if (FAILED(surface1->GetDC(FALSE, &dc))) {
            return nullptr;
        }
        return dc;
    }
    void d2d_unlock_bitmap(ID2D1Bitmap *bitmap, HDC dc) {
        // Try cast to ID2D1Bitmap1
        ComPtr<IDXGISurface1> surface1;
        ComPtr<IDXGISurface> surface;
        ComPtr<ID2D1Bitmap1> bitmap1;

        if (FAILED(bitmap->QueryInterface(bitmap1.GetAddressOf()))) {
            return;
        }
        if (FAILED(bitmap1->GetSurface(surface.GetAddressOf()))) {
            return;
        }
        if (FAILED(surface.As(&surface1))) {
            return;
        }
        surface1->ReleaseDC(nullptr);
    }

    class ID2D1SinkToBtkSink : public ID2D1GeometrySink {
        public:
            ID2D1SinkToBtkSink(PainterPathSink *s) : sink(s) {}

            HRESULT QueryInterface(REFIID riid, void **lp) override {
                if (riid == __uuidof(ID2D1GeometrySink)) {
                    *lp = this;
                    return S_OK;
                }
                if (riid == __uuidof(ID2D1SimplifiedGeometrySink)) {
                    *lp = this;
                    return S_OK;
                }
                return E_NOINTERFACE;
            }
            ULONG AddRef() override {
                return 1;
            }
            ULONG Release() override {
                return 0;
            }

            // ID2D1SimplifiedGeometrySink
            void SetFillMode(D2D1_FILL_MODE m) noexcept override {
                D2D_WARN("SetFillMode(D2D1_FILL_MODE m) not impl\n");
            }
            void SetSegmentFlags(D2D1_PATH_SEGMENT s) noexcept override {
                D2D_WARN("SetSegmentFlags(D2D1_PATH_SEGMENT s) not impl\n");
            }
            void BeginFigure(D2D1_POINT_2F p, D2D1_FIGURE_BEGIN f) noexcept override {
                if (f == D2D1_FIGURE_BEGIN_HOLLOW) {
                    D2D_WARN("f == D2D1_FIGURE_BEGIN_HOLLOW\n");
                }
                sink->move_to(p.x, p.y);
            }
            void EndFigure(D2D1_FIGURE_END f) noexcept override {
                if (f == D2D1_FIGURE_END_CLOSED) {
                    // Explict close
                    sink->close_path();
                }
            }
            void AddLines(const D2D1_POINT_2F *fp, UINT32 n) noexcept override {
                for (UINT32 i = 0; i < n; i ++) {
                    AddLine(fp[i]);
                }
            }
            void AddBeziers(const D2D1_BEZIER_SEGMENT *bs, UINT32 n) noexcept override {
                for (UINT32 i = 0; i < n; i ++) {
                    AddBezier(&bs[i]);
                }
            }
            void AddQuadraticBeziers(const D2D1_QUADRATIC_BEZIER_SEGMENT *qs, UINT32 n) noexcept override {
                for (UINT32 i = 0; i < n; i ++) {
                    AddQuadraticBezier(&qs[i]);
                }
            }
            HRESULT Close() noexcept override {
                return S_OK;
            }

            // ID2D1GeometrySink
            void AddLine(D2D1_POINT_2F point) noexcept override {
                sink->line_to(point.x, point.y);
            }
            void AddBezier(const D2D1_BEZIER_SEGMENT *b) noexcept override {
                sink->bezier_to(
                    b->point1.x, b->point1.y,
                    b->point2.x, b->point2.y,
                    b->point3.x, b->point3.y
                );
            }
            void AddQuadraticBezier(const D2D1_QUADRATIC_BEZIER_SEGMENT *q) noexcept override {
                sink->quad_to(
                    q->point1.x, q->point1.y,
                    q->point2.x, q->point2.y

                );
            }
            void AddArc(const D2D1_ARC_SEGMENT *a) noexcept override {
                D2D_WARN("AddArc(const D2D1_ARC_SEGMENT *a) not impl\n");
            }
        private:
            PainterPathSink *sink;
    };
}

BTK_NS_BEGIN

using Win32::DWrite;
using Win32::Wincodec;
using Win32::Direct2D;

class BrushImpl : public Refable <BrushImpl> , public Trackable {
    public:
        BrushImpl() = default;
        BrushImpl(const BrushImpl &b) {
            cmode = b.cmode;
            btype = b.btype;
            rect  = b.rect;
            data  = b.data;
        }
        // Slot for brush
        void painter_destroyed(PainterImpl *p);
        
        void set_color(const GLColor &c);
        void set_image(const PixBuffer &b);
        void set_gradient(const LinearGradient &g);
        void set_gradient(const RadialGradient &g);
        void reset_cache();

        // Map Painter => Actual brush, with painter connection
        std::map<
            PainterImpl *, 
            std::pair<Connection, ComPtr<ID2D1Brush> >
        >                 brushes;
        CoordinateMode    cmode = CoordinateMode::Relative;
        BrushType         btype = BrushType::Solid;
        FRect             rect  = {0.0f, 0.0f, 1.0f, 1.0f};

        std::variant<
            GLColor, 
            PixBuffer, 
            LinearGradient,
            RadialGradient,
            ComPtr<ID2D1Bitmap>  //< For textures
        >                 data = GLColor(0.0f, 0.0f, 0.0f, 1.0f);
        // For contain [Solid, Bitmap, LinearGradient, RadialGradient]
};
class TextureImpl : public Trackable {
    public:
        // Slot for texture
        void painter_destroyed();

        D2D1_BITMAP_INTERPOLATION_MODE mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
        ComPtr<ID2D1Bitmap>            bitmap;
        // < MinGW Crashed in GetSize() or GetPixelSize(), so we store this value
        D2D1_SIZE_U                    size;
};

class FontImpl : public Refable <FontImpl> {
    public:
        IDWriteTextFormat *lazy_eval();

        ComPtr<IDWriteTextFormat> format;
        u8string                  name; //< Family name
        float                     size    = 0; //< Size in points
        DWRITE_FONT_WEIGHT        weight  = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE         style   = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH       stretch = DWRITE_FONT_STRETCH_NORMAL;
};

class PenImpl : public Refable<PenImpl> {
    public:
        ID2D1StrokeStyle *lazy_eval();

        DashStyle                dstyle = DashStyle::Custom;
        LineJoin                 join = LineJoin::Miter;
        LineCap                  cap  = LineCap::Flat;
        std::vector<FLOAT>       dashes;
        ComPtr<ID2D1StrokeStyle> style;
        float                    dash_offset = 0.0f;
        float                    miter_limit = 10.0f;
};

class TextLayoutImpl : public Refable <TextLayoutImpl> {
    public:
        IDWriteTextLayout *lazy_eval();

        ComPtr<IDWriteTextLayout> layout; //< Layout of the text, created on need
        Ref<FontImpl>             font;   //< Font of the text
        u8string                  text;   //< Text to render
        float                     max_width = std::numeric_limits<float>::max(); //< Max width of the text
        float                     max_height = std::numeric_limits<float>::max(); //< Max height of the text
};

class PainterEffectImpl : public Trackable {
    public:
        ComPtr<ID2D1DeviceContext> context;
        ComPtr<ID2D1Effect>        effect;
        const GUID                *id = nullptr;
};

class PainterPathImpl {
    public:
        // Should I add this transform matrix to the path?
        D2D1::Matrix3x2F          mat = D2D1::Matrix3x2F::Identity();
        ComPtr<ID2D1PathGeometry> helper;
        ComPtr<ID2D1PathGeometry> path;
        ComPtr<ID2D1GeometrySink> sink;

        bool is_figure_open = false;
        bool is_double_open = false;
        bool is_open = false;

        float last_x = 0; //< Last Pen x
        float last_y = 0; //< Last Pen y
};

class PainterImpl {
    public:
        PainterImpl(HWND hwnd);
        PainterImpl(HDC hdc);
        PainterImpl(PixBuffer &bf);
        ~PainterImpl();

        void do_hr(HRESULT hr);
        void do_recreate();

        // Configure drawing color / brush
        void set_colorf(float r, float g, float b, float a = 1.0f);
        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void set_stroke_width(float width);
        void set_text_align(align_t align);
        void set_antialias(bool enable);
        void set_alpha(float alpha);
        void set_brush(BrushImpl *b);
        void set_font(FontImpl *f);
        void set_pen(PenImpl *p);

        void begin();
        void end();
        void clear();

        // Draw
        void draw_rect(float x, float y, float w, float h);
        void draw_line(float x1, float y1, float x2, float y2);
        void draw_circle(float x, float y, float r);
        void draw_ellipse(float x, float y, float xr, float yr);
        void draw_rounded_rect(float x, float y, float w, float h, float r);

        void draw_image(PainterEffectImpl *e, const FRect *dst, const FRect *src);
        void draw_image(TextureImpl *t,  const FRect *dst, const FRect *src);
        void draw_lines(const FPoint *fp, size_t n);
        void draw_text(u8string_view txt, float x, float y);
        void draw_text(IDWriteTextLayout *lay, float x, float y);

        void draw_path(PainterPathImpl *p);

        // Fill
        void fill_rect(float x, float y, float w, float h);
        void fill_circle(float x, float y, float r);
        void fill_ellipse(float x, float y, float xr, float yr);
        void fill_rounded_rect(float x, float y, float w, float h, float r);

        void fill_path(PainterPathImpl *p);
        void fill_mask(TextureImpl *t,  const FRect *dst, const FRect *src);

        // Scissor
        void push_scissor(float x, float y, float w, float h);
        void pop_scissor();

        // Transform
        void transform(const FMatrix &m);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float angle);
        void skew_x(float angle);
        void skew_y(float angle);
        void reset_transform();
        void push_transform();
        void pop_transform();
        auto transform_matrix() -> FMatrix;
        
        // Texture
        auto create_texture(PixFormat fmt, int w, int h) -> TextureImpl *;
        auto create_texture(TextureSource src, void *da) -> TextureImpl *;

        // Target 
        bool set_target(TextureImpl *tex);
        bool reset_target();

        // Notify 
        void notify_resize(int w, int h); //< Target size changed

        void initialize();

        static
        void Init();
        static
        void Shutdown();

        // Internal helpers
        auto get_brush_impl(const D2D1_RECT_F &area) -> ID2D1Brush *;
        auto get_brush(const D2D1_RECT_F &area) -> ID2D1Brush *;
        
        // Factorys
        auto alloc_texture(int w, int h, PixFormat fmt) -> TextureImpl *;

#if defined(BTK_DIRECT2D_ON_D3D11)
        // D3D11 Members with D2D1
        ComPtr<ID3D11Device> d3d_device;
        ComPtr<ID3D11DeviceContext> d3d_context;
        ComPtr<IDXGISwapChain> d3d_swapchain;
        ComPtr<ID3D11RenderTargetView> d3d_rendertarget;
        ComPtr<ID3D11Texture2D> d3d_backbuffer;
        ComPtr<ID2D1Device> d2d_device;
#endif

        ComPtr<ID2D1RenderTarget> target;
        ComPtr<ID2D1DeviceContext> context; //< Could be null if unsupported

        // Cached brushs
        struct {
            ComPtr<ID2D1SolidColorBrush> solid;
            ComPtr<ID2D1BitmapBrush> bitmap;
        } brushs;

        struct {
            D2D1::ColorF color = D2D1::ColorF::Black; // Current fill / stroke color
            ComPtr<ID2D1StrokeStyle> stroke_style; // Current stroke style
            ComPtr<IDWriteTextFormat> text_format; // Current text format
            float stroke_width = 1.0f; // Current stroke width
            float alpha = 1.0f; // Current alpha
            int n_clip_rects = 0; // Number of clip rects

            Ref<BrushImpl> brush; // Current logical brush
            Alignment text_align = Alignment::Left | Alignment::Top; // Current text aligns
        } state;

        // Information about current target
        struct {
            uint8_t bitmap_target : 1;
            uint8_t hwnd_target : 1;
            uint8_t d3d_target : 1;
            uint8_t dc_target : 1;
        } target_opt;

        union {
            struct {
                PixBuffer  *bf;
                IWICBitmap *wic_bitmap;
            }    bitmap; //< Target Bitmap
            HWND hwnd;   //< Target HWND
            HDC  hdc;    //< Target HDC
        } target_info;

        // Signal for cleanup any resources alloc by painter
        Signal<void()> signal_cleanup;

        // State of rendering target
        // std::stack<ComPtr<ID2D1RenderTarget>> target_stack;
        std::stack<D2D1::Matrix3x2F> matrix_stack;
};

inline PainterImpl::PainterImpl(HWND hwnd) {

    HRESULT hr;

#if defined(BTK_DIRECT2D_ON_D3D11)
    DXGI_SWAP_CHAIN_DESC desc = {};
    RECT rect;

    GetClientRect(hwnd, &rect);
    
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.Width = rect.right - rect.left;
    desc.BufferDesc.Height = rect.bottom - rect.top;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.RefreshRate.Numerator = 60;

    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    // Sample
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    // Output
    desc.OutputWindow = hwnd;
    desc.Windowed = TRUE;
    desc.Flags = 0;

    UINT dev_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
#if !defined(NDEBUG) && defined(_MSC_VER)
    dev_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        dev_flags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &desc,
        &d3d_swapchain,
        &d3d_device,
        nullptr,
        &d3d_context
    );

    if (FAILED(hr)) {
        BTK_THROW(std::runtime_error("Failed to create D3D11 device"));
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<ID2D1Factory1> factory1;
    hr = d3d_device.As(&dxgi_device);
    hr = Direct2D::GetInstance()->QueryInterface(IID_PPV_ARGS(&factory1));
    
    if (FAILED(hr)) {

    }

    hr = factory1->CreateDevice(dxgi_device.Get(), &d2d_device);

    if (FAILED(hr)) {
        BTK_THROW(std::runtime_error("Failed to create D2D device"));
    }

    hr = d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context);
    hr = context.As(&target);

    // Set Target
    ComPtr<IDXGISurface> dxgi_backbuffer;
    hr = d3d_swapchain->GetBuffer(0, IID_PPV_ARGS(&d3d_backbuffer));
    hr = d3d_swapchain->GetBuffer(0, IID_PPV_ARGS(&dxgi_backbuffer));

    // Create target
    ComPtr<ID2D1Bitmap1> bitmap;
    hr = context->CreateBitmapFromDxgiSurface(
        dxgi_backbuffer.Get(), 
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        ),
        &bitmap
    );
    context->SetTarget(bitmap.Get());
#else
    // Create a render target (legacy)
    hr = Direct2D::GetInstance()->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd),
        reinterpret_cast<ID2D1HwndRenderTarget**>(target.GetAddressOf())
    );
    do_hr(hr);
#endif

    // Set target options
    ZeroMemory(&target_opt, sizeof(target_opt));
    target_opt.hwnd_target = true;
    target_opt.d3d_target = true; //< Only valid on Direct2D on D3D11

    ZeroMemory(&target_info, sizeof(target_info));
    target_info.hwnd = hwnd;

    initialize();
}
inline PainterImpl::PainterImpl(HDC hdc) {
    HRESULT hr;

    // Create DC Target
    auto props = D2D1::RenderTargetProperties();
    hr = Direct2D::GetInstance()->CreateDCRenderTarget(
        &props,
        reinterpret_cast<ID2D1DCRenderTarget**>(target.GetAddressOf())
    );

    do_hr(hr);

    initialize();

    // Set target options
    ZeroMemory(&target_opt, sizeof(target_opt));
    target_opt.dc_target = true;

    ZeroMemory(&target_info, sizeof(target_info));
    target_info.hdc = hdc;

    initialize();
}
inline PainterImpl::PainterImpl(PixBuffer &bf) {
    // Make a temporary bitmap and render target
    ComPtr<IWICBitmap> bitmap;
    HRESULT hr;
    hr = Wincodec::GetInstance()->CreateBitmapFromMemory(
        bf.w(), bf.h(),
        GUID_WICPixelFormat32bppRGB,
        bf.pitch(),
        bf.h() * bf.pitch(),
        static_cast<BYTE*>(bf.pixels()),
        bitmap.GetAddressOf()
    );
    do_hr(hr);

    hr = Direct2D::GetInstance()->CreateWicBitmapRenderTarget(
        bitmap.Get(),
        D2D1::RenderTargetProperties(),
        reinterpret_cast<ID2D1RenderTarget**>(target.GetAddressOf())
    );

    initialize();

    // Set target options
    ZeroMemory(&target_opt, sizeof(target_opt));
    target_opt.bitmap_target = true;

    ZeroMemory(&target_info, sizeof(target_info));
    target_info.bitmap.bf = &bf;
    target_info.bitmap.wic_bitmap = bitmap.Detach();
}
inline PainterImpl::~PainterImpl() {
    signal_cleanup.emit();

    // Release bitmap if any
    if (target_opt.bitmap_target) {
        target_info.bitmap.wic_bitmap->Release();
    }
}
inline void PainterImpl::Init() {
    // Forward to 
    DWrite::Init();
    Wincodec::Init();
    Direct2D::Init();
}
inline void PainterImpl::Shutdown() {
    DWrite::Shutdown();
    Wincodec::Shutdown();
    Direct2D::Shutdown();
}
// Internal helpers
inline void PainterImpl::do_hr(HRESULT hr) {
    if (hr == D2DERR_RECREATE_TARGET) {
        signal_cleanup.emit();
    }
    if (FAILED(hr)) {
        abort();
    }
}
inline void PainterImpl::do_recreate() {
    // TODO : Recreate render target
    signal_cleanup.emit();
}
inline void PainterImpl::initialize() {
    HRESULT hr;
    // Make cached brushs
    hr = target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), brushs.solid.GetAddressOf());
    do_hr(hr);
    hr = target->CreateBitmapBrush(nullptr, D2D1::BitmapBrushProperties(), brushs.bitmap.GetAddressOf());
    do_hr(hr);

    // Make stroke style
    // hr = factory->CreateStrokeStyle(
    //     D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 1.0f, D2D1_DASH_STYLE_DOT, 0),
    //     nullptr,
    //     state.stroke_style.GetAddressOf()
    // );

    // Get device context if useable
    if (target_opt.hwnd_target) {
        target.As(&context);
    }
}
inline auto PainterImpl::get_brush_impl(const D2D1_RECT_F &r) -> ID2D1Brush* {
    if (!state.brush) {
        // User did not set brush, use color
        static_cast<void>(r);
        return brushs.solid.Get();
    }
    // OK user set brush, use it

    // First translate brush to target space
    D2D1_RECT_F brush_rect = r;
    BrushImpl *brush = state.brush.get();

    // TODO : Optimize this transformation

    switch (brush->cmode) {
        case CoordinateMode::Absolute : {
            // Absolute, just use it
            break;
        }
        case CoordinateMode::Relative : {
            // Depending on current mode, translate brush
            float width = brush_rect.right - brush_rect.left;
            float height = brush_rect.bottom - brush_rect.top;

            brush_rect.top += brush->rect.y * height;
            brush_rect.left += brush->rect.x * width;

            brush_rect.right = brush_rect.left + brush->rect.w * width;
            brush_rect.bottom = brush_rect.top + brush->rect.h * height;
            break;
        }
        case CoordinateMode::Device : {
            // Based on device, translate brush
            auto [w, h] = target->GetSize();

            brush_rect.top = brush->rect.y * h;
            brush_rect.left = brush->rect.x * w;

            brush_rect.right = brush_rect.left + brush->rect.w * w;
            brush_rect.bottom = brush_rect.top + brush->rect.h * h;
        }
    }

    auto translate_point = [&](FPoint p) -> FPoint {
        switch (brush->cmode) {
            case CoordinateMode::Absolute : {
                // Absolute, just use it
                return p;
            }
            case CoordinateMode::Relative : {
                // Depending on current mode, translate brush
                float width = brush_rect.right - brush_rect.left;
                float height = brush_rect.bottom - brush_rect.top;

                return FPoint(brush_rect.left + p.x * width,brush_rect.top + p.y * height);
            }
            case CoordinateMode::Device : {
                // Based on device, translate brush
                auto [w, h] = target->GetSize();

                return FPoint(p.x * w, p.y * h);
            }
        }
        abort();
    };
    auto translate_x = [&](float v) -> float {
        switch (brush->cmode) {
            case CoordinateMode::Absolute : {
                // Absolute, just use it
                return v;
            }
            case CoordinateMode::Relative : {
                // Depending on current mode, translate brush
                float width = brush_rect.right - brush_rect.left;
                float height = brush_rect.bottom - brush_rect.top;

                return v * width;
            }
            case CoordinateMode::Device : {
                // Based on device, translate brush
                auto [w, h] = target->GetSize();

                return v * w;
            }
        }
        abort();
    };
    auto translate_y = [&](float v) -> float {
        switch (brush->cmode) {
            case CoordinateMode::Absolute : {
                // Absolute, just use it
                return v;
            }
            case CoordinateMode::Relative : {
                // Depending on current mode, translate brush
                float width = brush_rect.right - brush_rect.left;
                float height = brush_rect.bottom - brush_rect.top;

                return v * height;
            }
            case CoordinateMode::Device : {
                // Based on device, translate brush
                auto [w, h] = target->GetSize();

                return v * h;
            }
        }
        // If we get here, we have a bug
        abort();
    };
    auto translate_stops = [&](const Gradient * g) -> ComPtr<ID2D1GradientStopCollection> {
        auto stops = g->stops();
        auto n = stops.size();

        // Alloc stops
        auto gs = (D2D1_GRADIENT_STOP*) _malloca(sizeof(D2D1_GRADIENT_STOP) * n);

        auto i = 0;
        for (auto [offset, color] : stops) {
            gs[i].position = offset;
            gs[i].color = D2D1::ColorF(color.r, color.g, color.b, color.a);
            i += 1;
        }

        ComPtr<ID2D1GradientStopCollection> col;
        target->CreateGradientStopCollection(
            gs,
            n,
            col.GetAddressOf()
        );

        _freea(gs);

        return col;
    };
    auto add_brush_to_cache = [&](ID2D1Brush *b) {
        // Connect signal
        auto con = signal_cleanup.connect(
            Btk::Bind(&BrushImpl::painter_destroyed, brush, this)
        );

        // Insert brush with signal connection
        brush->brushes.emplace(this, std::make_pair(con, b));
    };

    switch (brush->btype) {
        case BrushType::Solid : {
            // Set drawing color and return brush
            auto glcolor = std::get_if<GLColor>(&brush->data);
            brushs.solid->SetColor(D2D1::ColorF(
                glcolor->r,
                glcolor->g,
                glcolor->b,
                glcolor->a
            ));
            return brushs.solid.Get();
        }
        case BrushType::Bitmap : {
            // Bitmap, use bitmap brush
            auto buffer = std::get_if<PixBuffer>(&brush->data);
            auto iter = brush->brushes.find(this);
            // Create bitmap if needed
            ID2D1BitmapBrush *bitmap_brush;
            if (iter == brush->brushes.end()) {
                // No cached brush, create one
                ComPtr<ID2D1Bitmap> bitmap;
                HRESULT hr = target->CreateBitmap(
                    D2D1::SizeU(buffer->width(), buffer->height()),
                    buffer->pixels(),
                    buffer->pitch(),
                    D2D1::BitmapProperties(
                        D2D1::PixelFormat(
                            DXGI_FORMAT_R8G8B8A8_UNORM, //< TODO : Only some targets support this
                            D2D1_ALPHA_MODE_PREMULTIPLIED
                        )
                    ),
                    bitmap.GetAddressOf()
                );
                assert(SUCCEEDED(hr));

                // Create brush
                ComPtr<ID2D1BitmapBrush> dbush;
                hr = target->CreateBitmapBrush(
                    bitmap.Get(),
                    D2D1::BitmapBrushProperties(),
                    dbush.GetAddressOf()
                );

                add_brush_to_cache(dbush.Get());
                bitmap_brush = dbush.Get();
            }
            else {
                // Use cached brush
                bitmap_brush = static_cast<ID2D1BitmapBrush*>(iter->second.second.Get());
            }
            // Configure brush
            float target_width = brush_rect.right - brush_rect.left;
            float target_height = brush_rect.bottom - brush_rect.top;
            float w_scale = target_width / buffer->width();
            float h_scale = target_height / buffer->height();

            bitmap_brush->SetTransform(
                D2D1::Matrix3x2F::Scale(
                    w_scale,
                    h_scale
                )
                *
                D2D1::Matrix3x2F::Translation(
                    brush_rect.left,
                    brush_rect.top
                )
            );

            return bitmap_brush;
        }
        case BrushType::LinearGradient : {
            auto g = std::get_if<LinearGradient>(&brush->data);

            // Try query cache
            ID2D1LinearGradientBrush *g_brush;
            auto iter = brush->brushes.find(this);
            if (iter == brush->brushes.end()) {
                // No cached brush, create one
                auto gcol = translate_stops(g);

                ComPtr<ID2D1LinearGradientBrush> dbush;
                HRESULT hr = target->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(
                        D2D1::Point2F(),
                        D2D1::Point2F()
                    ),
                    gcol.Get(),
                    dbush.GetAddressOf()
                );

                add_brush_to_cache(dbush.Get());
                g_brush = dbush.Get();
            }
            else {
                // Use cached brush
                g_brush = static_cast<ID2D1LinearGradientBrush*>(iter->second.second.Get());
            }
            // Configure it
            auto start_point = translate_point(g->start_point());
            auto end_point = translate_point(g->end_point());

            g_brush->SetStartPoint(D2D1::Point2F(start_point.x, start_point.y));
            g_brush->SetEndPoint(D2D1::Point2F(end_point.x, end_point.y));

            return g_brush;
        }
        case BrushType::RadialGradient : {
            auto g = std::get_if<RadialGradient>(&brush->data);

            // Try query cache
            ID2D1RadialGradientBrush *g_brush;
            auto iter = brush->brushes.find(this);
            if (iter == brush->brushes.end()) {
                // No cached brush, create one
                ComPtr<ID2D1RadialGradientBrush> dbush;
                auto gcol = translate_stops(g);
                HRESULT hr = target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                        D2D1::Point2F(),
                        D2D1::Point2F(),
                        0.0f,
                        0.0f
                    ),
                    gcol.Get(),
                    dbush.GetAddressOf()
                );

                add_brush_to_cache(dbush.Get());
                g_brush = dbush.Get();
            }
            else {
                // Use cached brush
                g_brush = static_cast<ID2D1RadialGradientBrush*>(iter->second.second.Get());
            }
            // Configure it
            auto center = translate_point(g->center_point());
            auto radius_x = translate_x(g->radius_x());
            auto radius_y = translate_y(g->radius_y());
            auto offset = g->origin_offset();

            // Translate offset value
            switch (brush->cmode) {
                case CoordinateMode::Absolute : {
                    // Nothing to do
                    break;
                }
                case CoordinateMode::Device : {
                    [[fallthrough]];
                }
                case CoordinateMode::Relative : {
                    offset.x *= radius_x;
                    offset.y *= radius_y;
                    break;
                }
            }

            g_brush->SetCenter(D2D1::Point2F(center.x, center.y));
            g_brush->SetGradientOriginOffset(D2D1::Point2F(offset.x, offset.y));
            g_brush->SetRadiusX(radius_x);
            g_brush->SetRadiusY(radius_y);
            return g_brush;
        }
        default : {
            // Unsupported brush now:
            assert(!"Unsupported brush type");
        }
    }

    return nullptr;
}
inline auto PainterImpl::get_brush(const D2D1_RECT_F &r) -> ID2D1Brush* {
    auto brush = get_brush_impl(r);
    brush->SetOpacity(state.alpha);
    return brush;
}

// Begin / End
inline void PainterImpl::begin() {
    target->BeginDraw();

    // Reset transform
    target->SetTransform(D2D1::Matrix3x2F::Identity());
}
inline void PainterImpl::end() {
    assert(state.n_clip_rects == 0);
    assert(matrix_stack.empty());
    // assert(target_stack.empty());

    target->EndDraw();

#if defined(BTK_DIRECT2D_ON_D3D11)
    if (target_opt.d3d_target) {
        d3d_swapchain->Present(1, 0);
    }
#else
    if (0) {}
#endif
    else if (target_opt.bitmap_target) {
        // We need to copy the bitmap to pixbuffer
        IWICBitmap *bitmap = target_info.bitmap.wic_bitmap;
        PixBuffer  *buffer = target_info.bitmap.bf;

        // Copy back
        ComPtr<IWICBitmapLock> lock;
        HRESULT hr = bitmap->Lock(nullptr, WICBitmapLockRead, lock.GetAddressOf());
        do_hr(hr);

        // Get stride
        UINT stride;
        hr = lock->GetStride(&stride);
        do_hr(hr);

        assert(stride == buffer->pitch());

        // Get data
        BYTE *data;
        UINT  size;
        hr = lock->GetDataPointer(&size, &data);
        do_hr(hr);

        // Copy data
        assert(size == buffer->pitch() * buffer->h());
        Btk_memcpy(buffer->pixels(), data, size);
    }
}
inline void PainterImpl::clear() {
    target->Clear(state.color);
}

// Configure
inline void PainterImpl::set_colorf(float r, float g, float b, float a) {
    state.color = D2D1::ColorF(r, g, b, a);

    brushs.solid->SetColor(state.color);

    // Clear logical brush
    state.brush.reset();
}
inline void PainterImpl::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    set_colorf(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
inline void PainterImpl::set_stroke_width(float width) {
    state.stroke_width = width;
}
inline void PainterImpl::set_antialias(bool enable) {
    target->SetAntialiasMode(enable ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}
inline void PainterImpl::set_alpha(float alpha) {
    state.alpha = alpha;
}
inline void PainterImpl::set_brush(BrushImpl *brush) {
    state.brush = brush;
}
inline void PainterImpl::set_font(FontImpl *font) {
    state.text_format = font->lazy_eval();
}
inline void PainterImpl::set_pen(PenImpl *pen) {
    if (pen) {
        state.stroke_style = pen->lazy_eval();
    }
    else {
        state.stroke_style = nullptr;
    }
}
inline void PainterImpl::set_text_align(Alignment align) {
    state.text_align = align;
}

// Draw / fill 
inline void PainterImpl::draw_rect(float x, float y, float w, float h) {
    auto rect = D2D1::RectF(x, y, x + w, y + h);
    auto style = state.stroke_style.Get();
    auto brush = get_brush(rect);
    target->DrawRectangle(rect, brush, state.stroke_width, style);
}
inline void PainterImpl::draw_line(float x1, float y1, float x2, float y2) {
    auto rect = bounds_from_line(x1, y1, x2, y2);
    auto style = state.stroke_style.Get();
    auto brush = get_brush(rect);
    target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, state.stroke_width, style);
}
inline void PainterImpl::draw_circle(float x, float y, float r) {
    auto rect = bounds_from_circle(x, y, r);
    auto style = state.stroke_style.Get();
    auto brush = get_brush(rect);
    target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), r, r), brush, state.stroke_width, style);
}
inline void PainterImpl::draw_ellipse(float x, float y, float xr, float yr) {
    auto rect = bounds_from_ellipse(x, y, xr, yr);
    auto style = state.stroke_style.Get();
    auto brush = get_brush(rect);
    target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), xr, yr), brush, state.stroke_width, style);
}
inline void PainterImpl::draw_rounded_rect(float x, float y, float w, float h, float r) {
    auto rect = D2D1::RectF(x, y, x + w, y + h);
    auto style = state.stroke_style.Get();
    auto brush = get_brush(rect);
    target->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), r, r), brush, state.stroke_width, style);
}
inline void PainterImpl::draw_image(PainterEffectImpl *eff, const FRect *_dst, const FRect *_src) {
    // Incompleted, donot use it
    D2D1_RECT_F *p_src = nullptr;
    D2D1_RECT_F *p_dst = nullptr;

    D2D1_RECT_F src;
    D2D1_RECT_F dst;

    if (_dst) {
        p_dst = &dst;
        dst = D2D1::RectF(_dst->x, _dst->y, _dst->x + _dst->w, _dst->y + _dst->h);
    }
    if (_src) {
        p_src = &src;
        src = D2D1::RectF(_src->x, _src->y, _src->x + _src->w, _src->y + _src->h);
    }

    context->DrawImage(eff->effect.Get());
}
inline void PainterImpl::draw_image(TextureImpl *tex,  const FRect *dst, const FRect *src) {
    ID2D1Bitmap *bitmap = tex->bitmap.Get();
    D2D1_RECT_F *d2d_src = nullptr;
    D2D1_RECT_F *d2d_dst = nullptr;
    assert(bitmap);

    if (dst) {
        // d2d_dst = static_cast<D2D1_RECT_F*>(_alloca(sizeof(D2D1_RECT_F)));
        d2d_dst = Btk_stack_new(D2D1_RECT_F);
        *d2d_dst = D2D1::RectF(dst->x, dst->y, dst->x + dst->w, dst->y + dst->h);
    }

    if (src) {
        // d2d_src = static_cast<D2D1_RECT_F*>(_alloca(sizeof(D2D1_RECT_F)));
        d2d_src = Btk_stack_new(D2D1_RECT_F);
        *d2d_src = D2D1::RectF(src->x, src->y, src->x + src->w, src->y + src->h);
    }

    // // Zoom the image let it display at the right size
    // auto [tex_w, tex_h] = bitmap->GetSize();
    
    // float w_ratio = float(tex_w) / float(src.w);
    // float h_ratio = float(tex_h) / float(src.h);

    // // Transform the image to fit it
    // FRect tgt;

    // tgt.w = dst.w * w_ratio;
    // tgt.h = dst.h * h_ratio;

    // tgt.x = dst.x - (src.x / src.w) * dst.w;
    // tgt.y = dst.y - (src.y / src.h) * dst.h;

    // auto brush = brushs.bitmap.Get();

    // brush->SetBitmap(bitmap);
    // brush->SetTransform(
    //     D2D1::Matrix3x2F::Scale(tgt.w / tex_w, tgt.h / tex_h) *
    //     D2D1::Matrix3x2F::Translation(tgt.x, tgt.y)
    // );
    // // Draw actual area
    // target->FillRectangle(
    //     D2D1::RectF(dst.x, dst.y, dst.x + dst.w, dst.y + dst.h),
    //     brush
    // );

    // // Reset bitmap
    // brush->SetBitmap(nullptr);
    // D2D1_RECT_F d2d_src = D2D1::RectF(src.x, src.y, src.x + src.w, src.y + src.h);
    // D2D1_RECT_F d2d_dst = D2D1::RectF(dst.x, dst.y, dst.x + dst.w, dst.y + dst.h);

    target->DrawBitmap(bitmap, d2d_dst, state.alpha, tex->mode, d2d_src);
}
inline void PainterImpl::draw_lines(const FPoint *fp, size_t n) {
    // TODO : Optimize this
    for (size_t i = 0; i < n - 1; i++) {
        draw_line(fp[i].x, fp[i].y, fp[i + 1].x, fp[i + 1].y);
    }
}
inline void PainterImpl::draw_text(u8string_view txt, float x, float y) {
    if (state.text_format == nullptr) {
        D2D_WARN("PainterImpl::draw_text : No font was set\n");
        return;
    }

    auto u16 = txt.to_utf16();

    // New a text layout
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = DWrite::GetInstance()->CreateTextLayout(
        (LPWSTR) u16.c_str(),
        u16.length(),
        state.text_format.Get(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        layout.GetAddressOf()
    );

    // layout->SetFlowDirection(DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT);
    draw_text(layout.Get(), x, y);
}
inline void PainterImpl::draw_text(IDWriteTextLayout *layout, float x, float y) {
    // Get layout size
    // DWRITE_TEXT_METRICS metrics;
    // layout->GetMetrics(&metrics);

    align_t align = state.text_align;
    // HRESULT hr;

    // // Translate by align to left/top
    // if ((align & Alignment::Right) == Alignment::Right) {
    //     x -= metrics.width;
    // }
    // if ((align & Alignment::Bottom) == Alignment::Bottom) {
    //     y -= metrics.height;
    // }
    // if ((align & Alignment::Center) == Alignment::Center) {
    //     x -= metrics.width / 2;
    // }
    // if ((align & Alignment::Middle) == Alignment::Middle) {
    //     y -= metrics.height / 2;
    // }
    // if ((align & Alignment::Baseline) == Alignment::Baseline) {
    //     // Get layout baseline
    //     DWRITE_LINE_METRICS line_metrics;
    //     UINT line_count = 0;
    //     hr = layout->GetLineMetrics(&line_metrics, 1, &line_count);

    //     y -= line_metrics.baseline;
    // }

    // auto rect = D2D1::RectF(x, y, x + metrics.width, y + metrics.height);
    auto rect = layout_translate_by_align(layout, align, &x, &y);
    auto brush = get_brush(rect);

    // Draw it

    target->DrawTextLayout(D2D1::Point2F(x, y), layout, brush);
}
inline void PainterImpl::draw_path(PainterPathImpl *p) {
    D2D1::Matrix3x2F pmat;
    auto style = state.stroke_style.Get();
    auto path  = p->path.Get();
    auto &mat  = p->mat;
    bool trans = !mat.IsIdentity();

    D2D1_RECT_F rect;
    HRESULT hr;
    hr = path->GetBounds(nullptr, &rect);

    assert(SUCCEEDED(hr));

    auto brush = get_brush(rect);

    if (trans) {
        target->GetTransform(&pmat);
        target->SetTransform(mat *pmat);
    }
    target->DrawGeometry(path, brush, state.stroke_width, style);
    if (trans) {
        target->SetTransform(pmat);
    }
}

inline void PainterImpl::fill_rect(float x, float y, float w, float h) {
    auto rect = D2D1::RectF(x, y, x + w, y + h);
    auto brush = get_brush(rect);
    target->FillRectangle(rect, brush);
}
inline void PainterImpl::fill_circle(float x, float y, float r) {
    auto rect = bounds_from_circle(x, y, r);
    auto brush = get_brush(rect);
    target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), r, r), brush);
}
inline void PainterImpl::fill_ellipse(float x, float y, float xr, float yr) {
    auto rect = bounds_from_ellipse(x, y, xr, yr);
    auto brush = get_brush(rect);
    target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), xr, yr), brush);
}
inline void PainterImpl::fill_rounded_rect(float x, float y, float w, float h, float r) {
    auto rect = D2D1::RectF(x, y, x + w, y + h);
    auto brush = get_brush(rect);
    target->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), r, r), brush);
}

inline void PainterImpl::fill_path(PainterPathImpl *p) {
    D2D1::Matrix3x2F pmat;
    auto path = p->path.Get();
    auto &mat = p->mat;
    bool trans = !mat.IsIdentity();
    
    D2D1_RECT_F rect;
    HRESULT hr;
    hr = path->GetBounds(nullptr, &rect);
    auto brush = get_brush(rect);

    if (trans) {
        target->GetTransform(&pmat);
        target->SetTransform(mat *pmat);
    }
    target->FillGeometry(path, brush);
    if (trans) {
        target->SetTransform(pmat);
    }
}
inline void PainterImpl::fill_mask(TextureImpl *tex,  const FRect *_dst, const FRect *_src) {
    ID2D1Bitmap *mask = tex->bitmap.Get();
    FRect src;
    FRect dst;

    assert(mask);

    if (_dst) {
        dst = *_dst;
    }
    else {
        auto [w, h] = target->GetSize();
        dst = FRect(0, 0, w, h);
    }

    if (_src) {
        src = *_src;
    }
    else {
        auto [w, h] = mask->GetSize();
        src = FRect(0, 0, w, h);
    }

    D2D1_RECT_F d2d_src = D2D1::RectF(src.x, src.y, src.x + src.w, src.y + src.h);
    D2D1_RECT_F d2d_dst = D2D1::RectF(dst.x, dst.y, dst.x + dst.w, dst.y + dst.h);

    auto brush = get_brush(d2d_dst);

    target->FillOpacityMask(mask, brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, d2d_dst, d2d_src);
}

// Scissor
inline void PainterImpl::push_scissor(float x, float y, float w, float h) {
    target->PushAxisAlignedClip(D2D1::RectF(x, y, x + w, y + h), target->GetAntialiasMode());
    state.n_clip_rects += 1;
}
inline void PainterImpl::pop_scissor() {
    target->PopAxisAlignedClip();
    state.n_clip_rects -= 1;
}

// Transform
inline void PainterImpl::transform(const FMatrix &m) {
    D2D1::Matrix3x2F pmat;
    D2D1::Matrix3x2F mat = btkmat_to_d2d(m);

    target->GetTransform(&pmat);
    target->SetTransform(mat * pmat);
}
inline void PainterImpl::translate(float x, float y) {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    target->SetTransform(D2D1::Matrix3x2F::Translation(x, y) * mat);
}
inline void PainterImpl::scale(float x, float y) {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    target->SetTransform(D2D1::Matrix3x2F::Scale(x, y) * mat);
}
inline void PainterImpl::rotate(float angle) {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    target->SetTransform(D2D1::Matrix3x2F::Rotation(angle) * mat);
}
inline void PainterImpl::skew_x(float angle) {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    target->SetTransform(D2D1::Matrix3x2F::Skew(angle, 0) * mat);
}
inline void PainterImpl::skew_y(float angle) {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    target->SetTransform(D2D1::Matrix3x2F::Skew(0, angle) * mat);
}
inline void PainterImpl::reset_transform() {
    target->SetTransform(D2D1::Matrix3x2F::Identity());
}
inline void PainterImpl::push_transform() {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    matrix_stack.push(mat);
}
inline void PainterImpl::pop_transform() {
    assert(!matrix_stack.empty());
    auto mat = matrix_stack.top();
    matrix_stack.pop();
    target->SetTransform(mat);
}
inline auto PainterImpl::transform_matrix() -> FMatrix {
    D2D1::Matrix3x2F mat;
    target->GetTransform(&mat);
    return d2dmat_to_btk(mat);
}
// Texture
inline auto PainterImpl::create_texture(PixFormat fmt, int w, int h) -> TextureImpl *{
    auto d2d_fmt = btkfmt_to_d2d(fmt);

    ComPtr<ID2D1Bitmap> bitmap;
    HRESULT hr;

#if defined(_MSC_VER)
    if (context) {
        // Use new api ID2D1DeviceContext
        hr = context->CreateBitmap(
            D2D1::SizeU(w, h),
            nullptr,
            0,
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET,
                d2d_fmt
            ),
            reinterpret_cast<ID2D1Bitmap1**>(bitmap.GetAddressOf())
        );
    }
#else
    if (0) {}
#endif
    else {
        hr = target->CreateBitmap(
            D2D1::SizeU(w, h),
            D2D1::BitmapProperties(
                d2d_fmt
            ),
            bitmap.GetAddressOf()
        );
    }


    if (FAILED(hr)) {
        D2D_WARN("Failed to create bitmap");
        return nullptr;
    }

    auto tex = new TextureImpl;
    tex->bitmap = bitmap;
    tex->size.width  = w;
    tex->size.height = h;

    // Connect to painter
    signal_cleanup.connect(
        &TextureImpl::painter_destroyed, tex
    );

    return tex;
}
// inline auto PainterImpl::create_texture(TextureSource src, void *data) -> TextureImpl *{
//     target->CreateSharedBitmap(

//     );
// }

// Notify
inline void PainterImpl::notify_resize(int w, int h) {
    // Emm, In HIDPI Direct2D seem need framebuffer size, not logical size
    RECT rect;
    GetClientRect(target_info.hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

#if defined(BTK_DIRECT2D_ON_D3D11)
    if (target_opt.d3d_target) {
        // Unbind from old target
        context->SetTarget(nullptr);
        d3d_backbuffer.Reset();

        ComPtr<IDXGISurface> surface;
        ComPtr<ID2D1Bitmap1> bitmap;
        HRESULT hr = d3d_swapchain->ResizeBuffers(0, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        hr = d3d_swapchain->GetBuffer(0, IID_PPV_ARGS(&surface));
        hr = context->CreateBitmapFromDxgiSurface(
            surface.Get(),
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            ),
            bitmap.GetAddressOf()
        );
        context->SetTarget(bitmap.Get());
    }
#else
    if (0) {}
#endif
    else if (target_opt.hwnd_target) {
        static_cast<ID2D1HwndRenderTarget*>(target.Get())->Resize(D2D1::SizeU(w, h));
    }
}

// Target


// BrushImpl
inline void BrushImpl::painter_destroyed(PainterImpl *p) {
    auto iter = brushes.find(p);
    assert(iter != brushes.end());
    // Disconnect connection from painter
    iter->second.first.disconnect();
    // Remove brush
    brushes.erase(iter);
}
inline void BrushImpl::set_color(const GLColor &c) {
    reset_cache();
    data = c;
    btype = BrushType::Solid;
}
inline void BrushImpl::set_image(const PixBuffer &img) {
    reset_cache();
    data = img;
    btype = BrushType::Bitmap;
}
inline void BrushImpl::set_gradient(const LinearGradient & g) {
    reset_cache();
    data = g;
    btype = BrushType::LinearGradient;
}
inline void BrushImpl::set_gradient(const RadialGradient & g) {
    reset_cache();
    data = g;
    btype = BrushType::RadialGradient;
}
inline void BrushImpl::reset_cache() {
    for (auto &p : brushes) {
        // Disconnect all connections
        p.second.first.disconnect();
    }
    brushes.clear();
}

// FontImpl
inline auto FontImpl::lazy_eval() -> IDWriteTextFormat * {
    if (format == nullptr) {
        // Empty , create one
        HRESULT hr = DWrite::GetInstance()->CreateTextFormat(
            (LPWSTR) name.to_utf16().c_str(),
            nullptr,
            weight,
            style,
            stretch,
            size,
            L"",
            format.GetAddressOf()
        );

        if (FAILED(hr)) {
            D2D_WARN("FontImpl::lazy_eval : Failed to create text format");
        }
    }
    return format.Get();
}

// TextureImpl
inline void TextureImpl::painter_destroyed() {
    bitmap.Reset();
}

// TextLayout
inline auto TextLayoutImpl::lazy_eval() -> IDWriteTextLayout * {
    if (layout == nullptr) {
        // Create one
        assert(font);

        auto u16 = text.to_utf16();

        HRESULT hr = DWrite::GetInstance()->CreateTextLayout(
            (LPWSTR) u16.c_str(),
            u16.length(),
            font->lazy_eval(),
            max_width,
            max_height,
            layout.GetAddressOf()
        );

        if (FAILED(hr)) {
            D2D_WARN("TextLayoutImpl::lazy_eval : Failed to create text layout\n");
        }
    }
    return layout.Get();
}

// PenImpl
inline auto PenImpl::lazy_eval() -> ID2D1StrokeStyle *{
    if (style == nullptr) {
        D2D1_CAP_STYLE d2d_cap;
        D2D1_LINE_JOIN d2d_join;
        D2D1_DASH_STYLE d2d_style;
        switch (cap) {
            case LineCap::Flat: d2d_cap = D2D1_CAP_STYLE_FLAT; break;
            case LineCap::Round: d2d_cap = D2D1_CAP_STYLE_ROUND; break;
            case LineCap::Square: d2d_cap = D2D1_CAP_STYLE_SQUARE; break;
        }
        switch (join) {
            case LineJoin::Miter: d2d_join = D2D1_LINE_JOIN_MITER; break;
            case LineJoin::Round: d2d_join = D2D1_LINE_JOIN_ROUND; break;
            case LineJoin::Bevel: d2d_join = D2D1_LINE_JOIN_BEVEL; break;
        }
        switch (dstyle) {
            case DashStyle::Custom : d2d_style = D2D1_DASH_STYLE_CUSTOM; break;
            case DashStyle::Solid : d2d_style = D2D1_DASH_STYLE_SOLID; break;
            case DashStyle::Dash  : d2d_style = D2D1_DASH_STYLE_DASH; break;
            case DashStyle::Dot  : d2d_style = D2D1_DASH_STYLE_DOT; break;
            case DashStyle::DashDot  : d2d_style = D2D1_DASH_STYLE_DASH_DOT; break;
            case DashStyle::DashDotDot  : d2d_style = D2D1_DASH_STYLE_DASH_DOT_DOT; break;
        }

        HRESULT hr = Direct2D::GetInstance()->CreateStrokeStyle(
            D2D1::StrokeStyleProperties(
                d2d_cap,
                d2d_cap,
                d2d_cap,
                d2d_join,
                miter_limit,
                d2d_style,
                dash_offset
            ),
            dashes.data(),
            dashes.size(),
            style.GetAddressOf()
        );
        if (FAILED(hr)) {
            D2D_WARN("PenImpl::lazy_eval : Failed to create stroke style\n");
        }
    }
    return style.Get();
};



Painter::Painter() {
    priv = nullptr;
}
Painter::Painter(Painter &&p) {
    priv = p.priv;
    p.priv = nullptr;
}
Painter::~Painter() {
    delete priv;
}

// Painter Impl functions, Forward to PainterImpl
void Painter::begin() {
    priv->begin();
}
void Painter::end() {
    priv->end();
}
void Painter::clear() {
    priv->clear();
}
void Painter::set_colorf(float r, float g, float b, float a) {
    priv->set_colorf(r, g, b, a);
}
void Painter::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    priv->set_color(r, g, b, a);
}
void Painter::set_antialias(bool enable) {
    priv->set_antialias(enable);
}
void Painter::set_font(const Font &font) {
    priv->set_font(font.priv);
}
void Painter::set_pen(const Pen *pen) {
    if (pen) {
        priv->set_pen(pen->priv);
    }
    else {
        priv->set_pen(nullptr);
    }
}
void Painter::set_brush(const Brush &brush) {
    priv->set_brush(brush.priv);
}
void Painter::set_text_align(align_t align) {
    priv->set_text_align(align);
}
void Painter::set_stroke_width(float width) {
    priv->set_stroke_width(width);
}
void Painter::set_alpha(float alpha) {
    priv->set_alpha(alpha);
}
// 
void Painter::draw_rect(float x, float y, float w, float h) {
    priv->draw_rect(x, y, w, h);
}
void Painter::draw_line(float x1, float y1, float x2, float y2) {
    priv->draw_line(x1, y1, x2, y2);
}
void Painter::draw_circle(float x, float y, float r) {
    priv->draw_circle(x, y, r);
}
void Painter::draw_lines(const FPoint *fp, size_t n) {
    priv->draw_lines(fp, n);
}
void Painter::draw_rounded_rect(float x, float y, float w, float h, float r) {
    priv->draw_rounded_rect(x, y, w, h, r);
}
void Painter::draw_image(const Texture &tex, const FRect *dst, const FRect *src) {
    if (tex.empty()) {
        return;
    }
    priv->draw_image(tex.priv, dst, src);
}
void Painter::draw_text(u8string_view txt, float x, float y) {
    priv->draw_text(txt, x, y);
}
void Painter::draw_text(const TextLayout &lay, float x, float y) {
    if (lay.priv == nullptr) {
        return;
    }
    return priv->draw_text(lay.priv->lazy_eval(), x, y);
}
void Painter::draw_path(const PainterPath &path) {
    if (path.priv) {
        priv->draw_path(path.priv);
    }
}

void Painter::fill_rect(float x, float y, float w, float h) {
    priv->fill_rect(x, y, w, h);
}
void Painter::fill_circle(float x, float y, float r) {
    priv->fill_circle(x, y, r);
}
void Painter::fill_ellipse(float x, float y, float xr, float yr) {
    priv->fill_ellipse(x, y, xr, yr);
}
void Painter::fill_rounded_rect(float x, float y, float w, float h, float r) {
    priv->fill_rounded_rect(x, y, w, h, r);
}
void Painter::fill_path(const PainterPath &path) {
    if (path.priv) {
        priv->fill_path(path.priv);
    }
}
void Painter::fill_mask(const Texture &mask, const FRect *dst, const FRect *src) {
    if (mask.empty()) {
        return;
    }
    priv->fill_mask(mask.priv, dst, src);
}

void Painter::push_scissor(float x, float y, float w, float h) {
    priv->push_scissor(x, y, w, h);
}
void Painter::pop_scissor() {
    priv->pop_scissor();
}

void Painter::transform(const FMatrix &m) {
    priv->transform(m);
}
void Painter::translate(float x, float y) {
    priv->translate(x, y);
}
void Painter::scale(float x, float y) {
    priv->scale(x, y);
}
void Painter::rotate(float angle) {
    priv->rotate(angle);
}
void Painter::skew_x(float angle) {
    priv->skew_x(angle);
}
void Painter::skew_y(float angle) {
    priv->skew_y(angle);
}
void Painter::reset_transform() {
    priv->reset_transform();
}
void Painter::push_transform() {
    priv->push_transform();
}
void Painter::pop_transform() {
    priv->pop_transform();
}
auto Painter::transform_matrix() -> FMatrix {
    return priv->transform_matrix();
}

auto Painter::create_texture(PixFormat fmt, int w, int h) -> Texture {
    Texture t;
    t.priv = priv->create_texture(fmt, w, h);
    return t;
}
auto Painter::create_texture(const PixBuffer &buf) -> Texture {
    Texture t = create_texture(buf.format(), buf.width(), buf.height());
    t.update(
        nullptr,
        buf.pixels(),
        buf.pitch()
    );
    return t;
}


void Painter::notify_resize(int w, int h) {
    priv->notify_resize(w, h);
}
void Painter::operator =(Painter && p) {
    delete priv;
    priv = p.priv;
    p.priv = nullptr;
}

Painter Painter::FromWindow(AbstractWindow *w) {
    HWND hwnd;
    if (w->query_value(AbstractWindow::Hwnd, &hwnd)) {
        return FromHwnd(hwnd);
    }
    // ???
    abort();
}
Painter Painter::FromHwnd(void * hwnd) {
    Painter p;
    p.priv = new PainterImpl(reinterpret_cast<HWND>(hwnd));
    return p;
}
Painter Painter::FromHdc(void *hdc) {
    Painter p;
    p.priv = new PainterImpl(reinterpret_cast<HDC>(hdc));
    return p;
}
Painter Painter::FromPixBuffer(PixBuffer &buf) {
    Painter p;
    p.priv = new PainterImpl(buf);
    return p;
}
void    Painter::Init() {
    PainterImpl::Init();
}
void    Painter::Shutdown() {
    PainterImpl::Shutdown();

}

// Texture
Texture::Texture() {
    priv = nullptr;
}
Texture::Texture(Texture &&t) {
    priv = t.priv;
    t.priv = nullptr;
}
Texture::~Texture() {
    delete priv;
}
bool Texture::empty() const {
    if (priv) {
        // Internal data already destroyed
        return priv->bitmap == nullptr;
    }
    // No texture
    return true;
}
void Texture::clear() {
    delete priv;
    priv = nullptr;
}
void Texture::update(const Rect *where, cpointer_t data, uint32_t pitch) {
    D2D1_RECT_U *rect = nullptr;
    if (where) {
        rect = (D2D1_RECT_U*) _malloca(sizeof(D2D1_RECT_F));
        rect->left = where->x;
        rect->top = where->y;
        rect->right = where->x + where->w;
        rect->bottom = where->y + where->h;
    }
    HRESULT hr = priv->bitmap->CopyFromMemory(rect, data, pitch);
    if (FAILED(hr)) {
        D2D_WARN("Failed to update texture: ");
    }
    _freea(rect);
}
Size Texture::size() const {
    // MinGW Crashed on bitmap->GetSize()
    // auto [w, h] = priv->bitmap->GetPixelSize();
    if (!priv) {
        return Size(0, 0);
    }

    auto [w, h] = priv->size;
    return Size(w, h);
}
void Texture::set_interpolation_mode(InterpolationMode m) {
    D2D1_BITMAP_INTERPOLATION_MODE mode;
    switch (m) {
        case InterpolationMode::Nearest : mode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR; break;
        case InterpolationMode::Linear : mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR; break;
    }

    if (!empty()) {
        priv->mode = mode;
    }
}

Texture &Texture::operator =(Texture &&t) {
    delete priv;
    priv = t.priv;
    t.priv = nullptr;
    return *this;
}
Texture &Texture::operator =(std::nullptr_t) {
    clear();
    return *this;
}

// Font
COW_IMPL(Font);

Font::Font() {
    priv = nullptr;
}
Font::Font(u8string_view name, float size) {
    priv = new FontImpl;
    priv->name = name;
    priv->size = size;

    priv->ref();
}
float Font::size() const {
    if (priv) {
        return priv->size;
    }
    return 0;
}
void  Font::set_size(float size) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    priv->size = size;
}
void  Font::set_bold(bool bold) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    if (bold) {
        priv->weight = DWRITE_FONT_WEIGHT_BOLD;
    }
    else {
        priv->weight = DWRITE_FONT_WEIGHT_BOLD;
    }
}
void  Font::set_italic(bool italic) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    if (italic) {
        priv->style = DWRITE_FONT_STYLE_ITALIC;
    }
    else {
        priv->style = DWRITE_FONT_STYLE_NORMAL;
    }
}
void  Font::set_family(u8string_view family) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    priv->name = family;
}
bool  Font::empty() const {
    return priv == nullptr;
}
auto  Font::ListFamily() -> StringList {
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
    std::wstring tmp;
    
    ComPtr<IDWriteFontCollection> col;
    StringList ret;
    HRESULT hr = S_OK;
    BOOL exists = FALSE;
    BOOL has_local = FALSE;

    has_local = GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

    hr = DWrite::GetInstance()->GetSystemFontCollection(&col);
    if (FAILED(hr)) {
        return ret;
    }

    UINT32 n = col->GetFontFamilyCount();
    ret.reserve(n);

    for (UINT32 i = 0; i < n; i ++) {
        ComPtr<IDWriteFontFamily>    family;
        ComPtr<IDWriteLocalizedStrings> str;
        hr = col->GetFontFamily(i, &family);
        hr = family->GetFamilyNames(&str);

        // Find local string
        UINT32 index = 0;
        if (has_local) {
            hr = str->FindLocaleName(locale_name, &index, &exists);
        }
        if (SUCCEEDED(hr) || !exists) {
            // No name, try en
            hr = str->FindLocaleName(L"en-us", &index, &exists);
        }
        if (!exists) {
            // Oh no
            index = 0;
        }

        UINT32 len;
        hr = str->GetStringLength(index, &len);

        tmp.resize(len);

        hr = str->GetString(index, tmp.data(), len + 1);

        if (SUCCEEDED(hr)) {
            ret.emplace_back(u8string::from(tmp));
        }
    }
    return ret;
}

// TextLayout
COW_IMPL(TextLayout);

TextLayout::TextLayout() {
    priv = nullptr;
}
void TextLayout::set_text(u8string_view txt) {
    begin_mut();
    priv->layout.Reset(); //< Clear previous layout
    priv->text = txt;
}
void TextLayout::set_font(const Font &fnt) {
    begin_mut();
    priv->layout.Reset(); //< Clear previous layout
    priv->font = fnt.priv;
}
FSize TextLayout::size() const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_TEXT_METRICS m;
            layout->GetMetrics(&m);
            // This size include white space
            return FSize(m.widthIncludingTrailingWhitespace, m.height);
        }
    }
    return FSize(-1, -1);
}
size_t TextLayout::line() const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_TEXT_METRICS m;
            layout->GetMetrics(&m);
            // This size include white space
            return m.lineCount;
        }
    }
    return 0;
}
bool TextLayout::hit_test(float x, float y, TextHitResult *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            BOOL inside;
            BOOL is_trailing_hit;
            DWRITE_HIT_TEST_METRICS m;
            if (FAILED(layout->HitTestPoint(x, y, &is_trailing_hit, &inside, &m))) {
                return false;
            }
            if (res) {
                res->text = m.textPosition;
                res->length = m.length;

                res->box.x = m.left;
                res->box.y = m.top;
                res->box.w = m.width;
                res->box.h = m.height;

                res->trailing = is_trailing_hit;
                res->inside = inside;
            }

            return true;
        }
    }
    return false;
}
bool TextLayout::hit_test_pos(size_t pos, bool trailing_hit, float *x, float *y, TextHitResult *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_HIT_TEST_METRICS m;
            if (FAILED(layout->HitTestTextPosition(pos, trailing_hit, x, y, &m))) {
                return false;
            }
            if (res) {
                res->text = m.textPosition;
                res->length = m.length;

                res->box.x = m.left;
                res->box.y = m.top;
                res->box.w = m.width;
                res->box.h = m.height;

                // res->trailing = is_trailing_hit;
                // res->inside = inside;
            }

            return true;
        }
    }
    return false;
}
bool TextLayout::hit_test_range(size_t text, size_t len, float org_x, float org_y, TextHitResults *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            HRESULT hr;
            UINT32 count = 0;

            DWRITE_HIT_TEST_METRICS m;

            hr = layout->HitTestTextRange(text, len, org_x, org_y, &m, 0, &count);
            if (FAILED(hr) && hr != E_NOT_SUFFICIENT_BUFFER) {
                D2D_WARN("HitTestTextRange failed %d", hr);
                return false;
            }

            // Alloc the result array
            auto *hit_res = (DWRITE_HIT_TEST_METRICS*) _malloca(sizeof(DWRITE_HIT_TEST_METRICS) * count);
            hr = layout->HitTestTextRange(text, len, org_x, org_y, hit_res, count, &count);
            if (FAILED(hr)) {
                _freea(hit_res);
                return false;
            }

            // Copy the result
            if (res) {
                res->resize(count);
                for (size_t i = 0; i < count; i++) {
                    (*res)[i].text = hit_res[i].textPosition;
                    (*res)[i].length = hit_res[i].length;

                    (*res)[i].box.x = hit_res[i].left;
                    (*res)[i].box.y = hit_res[i].top;
                    (*res)[i].box.w = hit_res[i].width;
                    (*res)[i].box.h = hit_res[i].height;

                    // Unused properties
                    (*res)[i].trailing = true;
                    (*res)[i].inside = true;
                }
            }
            _freea(hit_res);
            return true;
        }
    }
    return false;
}
bool TextLayout::line_metrics(TextLineMetricsList *metrics) const {
    if (priv) {
        auto layout = priv->lazy_eval();

        DWRITE_LINE_METRICS m;
        HRESULT hr;
        UINT32 count = 0;

        hr = layout->GetLineMetrics(&m, 0, &count);
        if (FAILED(hr) && hr != E_NOT_SUFFICIENT_BUFFER) {
            D2D_WARN("GetLineMetrics failed %d", hr);
            return false;
        }

        // Alloc the result array
        auto *line_res = (DWRITE_LINE_METRICS*) _malloca(sizeof(DWRITE_LINE_METRICS) * count);
        hr = layout->GetLineMetrics(line_res, count, &count);
        if (FAILED(hr)) {
            _freea(line_res);
            return false;
        }

        // Copy result
        if (metrics) {
            metrics->resize(count);
            for (size_t i = 0; i < count; i++) { 
                (*metrics)[i].baseline = line_res[i].baseline;
                (*metrics)[i].height   = line_res[i].height;
            }
        }

        _freea(line_res);
        return true;
    }
    return false;
}

// Brush
COW_IMPL(Brush);

Brush::Brush() {
    priv = nullptr;
}
void Brush::set_color(const GLColor &c) {
    begin_mut();
    priv->set_color(c);
}
void Brush::set_image(const PixBuffer &b) {
    begin_mut();
    priv->set_image(b);
}
void Brush::set_gradient(const LinearGradient &g) {
    begin_mut();
    priv->set_gradient(g);
}
void Brush::set_gradient(const RadialGradient &g) {
    begin_mut();
    priv->set_gradient(g);
}

BrushType Brush::type() const {
    if (priv) {
        return priv->btype;
    }
    return BrushType::Solid;
}
GLColor   Brush::color() const {
    if (priv) {
        if (priv->btype == BrushType::Solid) {
            return std::get<GLColor>(priv->data);
        }
    }
    return Color::Black;
}

// PainterEffect

// PainterEffect::PainterEffect() {
//     priv = nullptr;
// }
// PainterEffect::PainterEffect(PainterEffect &&e) {
//     priv = e.priv;
//     e.priv = nullptr;
// }
// PainterEffect::PainterEffect(EffectType t) {
//     priv = new PainterEffectImpl;
//     switch (t) {
//         case EffectType::Blur : priv->id = &CLSID_D2D1GaussianBlur; break; 
//     }
// }
// PainterEffect::~PainterEffect() {
//     delete priv;
// }

// bool PainterEffect::attach(const Painter &p) {
//     if (priv) {
//         if (p.priv->context) {
//             if (priv->context == p.priv->context) {
//                 return true;
//             }
//             priv->context = p.priv->context;
//             return priv->context->CreateEffect(
//                 *priv->id,
//                 &priv->effect
//             ) == S_OK;
//         }
//         // Not d3d backend, unsupported
//         return false;
//     }
//     return false;
// }
// bool PainterEffect::set_input(const Texture &tex) {
//     if (priv && !tex.empty()) {
//         priv->effect->SetInput(0, tex.priv->bitmap.Get());
//         return true;
//     }
//     return false;
// }
// bool PainterEffect::set_value(EffectParam param, ...) {
//     va_list varg;
//     va_start(varg, param);

//     bool ret = true;

//     switch (param) {
//         case EffectParam::BlurStandardDeviation : {
//             ret = SUCCEEDED(priv->effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, va_arg(varg, float)));
//             break;
//         }
//         default : {
//             ret = false;
//             break;
//         }
//     }

//     va_end(varg);
//     return ret;
// }
// Texture PainterEffect::output() const {
//     if (priv) {
//         ComPtr<ID2D1Bitmap> bitmap;
//         ComPtr<ID2D1Image> image;
//         priv->effect->GetOutput(&image);

//         if (SUCCEEDED(image.As(&bitmap))) {
//             Texture tex;
//             tex.priv = new TextureImpl;
//             tex.priv->bitmap = bitmap;
//             return tex;
//         }
//     }
//     return Texture();
// }
// PainterEffect &PainterEffect::operator =(PainterEffect &&e) {
//     if (&e != this) {
//         delete priv;
//         priv = e.priv;
//         e.priv = nullptr;
//     }
//     return *this;
// }
// PainterEffect &PainterEffect::operator =(std::nullptr_t) {
//     delete priv;
//     priv = nullptr;
//     return *this;
// }
// PainterPath

PainterPath::PainterPath() {
    priv = nullptr;
}
PainterPath::PainterPath(PainterPath &&p) {
    priv = p.priv;
    p.priv = nullptr;
}
PainterPath::~PainterPath() {
    delete priv;
}
void PainterPath::open() {
    if (!priv) {
        // Not yet initialized
        priv = new PainterPathImpl;

        // Call factory to create it
        HRESULT hr;
        hr = Direct2D::GetInstance()->CreatePathGeometry(&priv->path);
        if (FAILED(hr)) {
            D2D_WARN("CreatePathGeometry failed %d", hr);
            BTK_THROW(std::runtime_error("CreatePathGeometry failed"));
        }
    }

    HRESULT hr;
    if (priv->is_double_open) {
        // Double open, combine mode
        hr = Direct2D::GetInstance()->CreatePathGeometry(&priv->helper);
        hr = priv->helper->Open(&priv->sink);
    }
    else {
        hr = priv->path->Open(
            &priv->sink
        );
    }
    if (FAILED(hr)) {
        D2D_WARN("Open failed %d", hr);
        BTK_THROW(std::runtime_error("Open failed"));
    }
    priv->is_open = true;
}
void PainterPath::close() {
    HRESULT hr;
    // If not open, do nothing
    if (priv->is_figure_open) {
        priv->sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    hr = priv->sink->Close();
    if (FAILED(hr)) {
        D2D_WARN("Close failed %d", hr);
        BTK_THROW(std::runtime_error("Close failed"));
    }
    if (priv->is_double_open) {
        // Conbime
        ComPtr<ID2D1PathGeometry> dst;
        ComPtr<ID2D1GeometrySink> sink;
        hr = Direct2D::GetInstance()->CreatePathGeometry(&dst);

        hr = dst->Open(&sink);
        hr = priv->path->CombineWithGeometry(priv->helper.Get(), D2D1_COMBINE_MODE_UNION, nullptr, sink.Get());
        hr = sink->Close();

        priv->helper.Reset();
        priv->path = dst;
    }
    priv->sink.Reset();
    priv->is_figure_open = false;
    priv->is_double_open = true; //< Double open impl by combine path
    priv->is_open = false;
    priv->last_x = 0;
    priv->last_x = 0    ;
}
void PainterPath::move_to(float x, float y) {
    // Start a new figure if not open
    if (priv->is_figure_open) {
        priv->sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    priv->sink->BeginFigure(
        D2D1::Point2F(x, y),
        D2D1_FIGURE_BEGIN_FILLED
    );
    priv->is_figure_open = true;
    priv->last_x = x;
    priv->last_x = y;
}
void PainterPath::line_to(float x, float y) {
    priv->sink->AddLine(D2D1::Point2F(x, y));
    priv->last_x = x;
    priv->last_x = y;
}
void PainterPath::quad_to(float x1, float y1, float x2, float y2) {
    priv->sink->AddQuadraticBezier(
        D2D1::QuadraticBezierSegment(
            D2D1::Point2F(x1, y1),
            D2D1::Point2F(x2, y2)
        )
    );
    priv->last_x = x2;
    priv->last_x = y2;
}
void PainterPath::bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) {
    priv->sink->AddBezier(
        D2D1::BezierSegment(
            D2D1::Point2F(x1, y1),
            D2D1::Point2F(x2, y2),
            D2D1::Point2F(x3, y3)
        )
    );
    priv->last_x = x3;
    priv->last_x = y3;
}
void PainterPath::arc_to(float x1, float y1, float x2, float y2, float radius) {
    BTK_ONCE(D2D_WARN("NanoVG impl for arc_to\n"));
    fallback_painter_arc_to(this, priv->last_x, priv->last_y, x1, y1, x2, y2, radius);
}
void PainterPath::close_path() {
    priv->sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    priv->is_figure_open = false;
}
void PainterPath::clear() {
    delete priv;
    priv = nullptr;
}
FRect PainterPath::bounding_box() const {
    if (priv) {
        D2D1_RECT_F bbox;
        HRESULT hr = priv->path->GetBounds(
            &priv->mat,
            &bbox
        );
        if (FAILED(hr)) {
            D2D_WARN("GetBounds failed %d", hr);
            BTK_THROW(std::runtime_error("GetBounds failed"));
        }
        return FRect(bbox.left, bbox.top, bbox.right - bbox.left, bbox.bottom - bbox.top);
    }
    return FRect(0, 0, 0, 0);
}
bool  PainterPath::contains(float x, float y) const {
    if (priv) {
        BOOL contains;
        priv->path->FillContainsPoint(D2D1::Point2F(x, y), priv->mat, &contains);
        return contains;
    }
    return false;
}
void  PainterPath::stream(PainterPathSink *s) const {
    ID2D1SinkToBtkSink sink(s);
    HRESULT hr = priv->path->Stream(&sink);
    if (FAILED(hr)) {
        D2D_WARN("ID2D1PathGeo stream() failed\n");
    }
}
void  PainterPath::set_transform(const FMatrix &mat) {
    if (priv) {
        priv->mat = btkmat_to_d2d(mat);
    }
}
void  PainterPath::set_winding(PathWinding winding) {
    if (priv) {
        D2D1_FILL_MODE mode;
        switch (winding) {
            case PathWinding::CW  : mode = D2D1_FILL_MODE_ALTERNATE; break;
            case PathWinding::CCW : mode = D2D1_FILL_MODE_WINDING;   break;
        }
        priv->sink->SetFillMode(mode);
    }
}

PainterPath &PainterPath::operator =(PainterPath &&p) {
    delete priv;
    priv = p.priv;
    p.priv = nullptr;
    return *this;
}

// Pen
COW_IMPL(Pen);
Pen::Pen() {
    priv = nullptr;
}
void Pen::set_dash_pattern(const float *dahes, size_t n) {
    begin_mut();
    priv->style.Reset();
    priv->dashes.assign(dahes, dahes + n);
    priv->dstyle = DashStyle::Custom;
}
void Pen::set_dash_offset(float offset) {
    begin_mut();
    priv->style.Reset();
    priv->dash_offset = offset;
}
void Pen::set_dash_style(DashStyle style) {
    begin_mut();
    priv->dashes.clear();
    priv->dstyle = style;
}
void Pen::set_miter_limit(float limit) {
    begin_mut();
    priv->style.Reset();
    priv->miter_limit = limit;
}
void Pen::set_line_join(LineJoin join) {
    begin_mut();
    priv->style.Reset();
    priv->join = join;
}
void Pen::set_line_cap(LineCap cap) {
    begin_mut();
    priv->style.Reset();
    priv->cap = cap;
}

BTK_NS_END