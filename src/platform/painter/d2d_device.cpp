#include "build.hpp"
#include "common/win32/direct2d.hpp"
#include "common/win32/wincodec.hpp"
#include "common/win32/dwrite.hpp"

#include <Btk/detail/reference.hpp>
#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <Btk/font.hpp>
#include <unordered_map>
#include <windows.h>
#include <d3d11.h>
#include <wrl.h>

#undef min
#undef max

// Helper for Impl PaintResource interface
#define BTK_MAKE_PAINT_RESOURCE      \
    public:                          \
        UINT _refcount = 0;          \
        void ref() override final {  \
            ++_refcount;             \
        }                            \
        void unref() override final {\
            if (--_refcount == 0) {  \
                delete this;         \
            }                        \
        }                            \
    public:                          \

BTK_PRIV_BEGIN

// Forward declarations
using Win32::WincodecInitializer;
using Win32::Direct2DInitializer;
using Win32::Wincodec;
using Win32::Direct2D;

using Microsoft::WRL::ComPtr;

class D2DDeviceContext;
class D2DRenderTarget;
class D2DPaintDevice;

/**
 * @brief Base D2D1RenderTarget wrapper class
 * 
 */
class D2DRenderTarget  : public PaintContext {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DRenderTarget(PaintDevice *device, ID2D1RenderTarget *tar);
        ~D2DRenderTarget() override;

        // Inherit from PaintResource
        auto signal_destroyed() -> Signal<void()> & override;

        // Inherit from GraphicsContext
        void begin() override;
        void end() override;
        void swap_buffers() override;

        // Inhert from PaintContext
        void clear(Brush &) override;

        // Draw
        bool draw_path(const PainterPath &path) override;
        bool draw_line(float x1, float y1, float x2, float y2) override;
        bool draw_rect(float x, float y, float w, float h) override;
        bool draw_rounded_rect(float x, float y, float w, float h, float r) override;
        bool draw_ellipse(float x, float y, float xr, float yr) override;
        bool draw_image(AbstractTexture *image, const FRect *dst, const FRect *src) override;

        // Text
        bool draw_text(Alignment, Font &font, u8string_view text, float x, float y) override;
        bool draw_text(Alignment, const TextLayout &layout      , float x, float y) override;

        // Fill
        bool fill_path(const PainterPath &path) override;
        bool fill_rect(float x, float y, float w, float h) override;
        bool fill_rounded_rect(float x, float y, float w, float h, float r) override;
        bool fill_ellipse(float x, float y, float xr, float yr) override;
        bool fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) override;

        // Texture
        auto create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> AbstractTexture * override;;

        bool set_state(PaintContextState state, const void *data) override;
        bool native_handle(PaintContextHandle, void *out) override { return false; }
        

        auto get_brush(const FRect &area) -> ID2D1Brush *;
        auto get_path(const PainterPath &) -> ID2D1PathGeometry *;
        auto get_pen()   -> ID2D1StrokeStyle *;
    protected:
        // Create Gradient stops from the gradient
        auto map_gradient(const Gradient &) -> ComPtr<ID2D1GradientStopCollection>;
        auto map_linear_brush(Brush &, const FRect &area) -> ID2D1Brush *;
        auto map_radial_brush(Brush &, const FRect &area) -> ID2D1Brush *;
        auto map_bitmap_brush(Brush &, const FRect &area) -> ID2D1Brush *;
        auto map_brush(Brush &, const FRect &area) -> ID2D1Brush *;

        void draw_text_layout(Alignment alig, IDWriteTextLayout *lay, float x, float y);

        PaintDevice                 *paint_device; //< Device belong to

        ComPtr<ID2D1RenderTarget>    target;
        Signal<void()>               signal; //< Signal when destroyed & losted
        ComPtr<ID2D1SolidColorBrush> solid_brush; //< Color Brush
        ComPtr<ID2D1BitmapBrush>     bitmap_brush; //< Bitmap Brush

        // Current state
        float                        stroke_width = 1.0f; //< Current Stroke Width
        float                        alpha = 1.0f; //< Global Alpha
        bool                         has_scissor = false; //< Has Scissor rectangle pushed
        Brush                        brush;        //< Current brush
        Pen                          pen;          //< Current pen
    friend class D2DTexture;
};
/**
 * @brief Draw bitmap to 
 * 
 */
class D2DBitmapTarget final : public D2DRenderTarget {
    public:
        /**
         * @brief Construct a new D2DBitmapTarget object
         * 
         * @param target The bitmap target
         * @param bitmap The dst bitmap 
         */
        D2DBitmapTarget(PaintDevice *device, ID2D1BitmapRenderTarget *target, ID2D1Bitmap *bitmap) : 
            D2DRenderTarget(device, target), bitmap_target(target), dst(bitmap) { }

        void swap_buffers() override {
            ComPtr<ID2D1Bitmap> src;
            HRESULT hr;
            hr = bitmap_target->GetBitmap(src.GetAddressOf());
            hr = dst->CopyFromBitmap(nullptr, src.Get(), nullptr);
        }
    private:
        ComPtr<ID2D1BitmapRenderTarget> bitmap_target;
        ComPtr<ID2D1Bitmap>             dst;
};
/**
 * @brief Wic Bitmap Target
 * 
 */
class D2DWicBitmapTarget final : public D2DRenderTarget {
    public:
        D2DWicBitmapTarget(PaintDevice *device, ID2D1RenderTarget *target, PixBuffer *pixbuffer, IWICBitmap *bitmap) :
            D2DRenderTarget(device, target), pixbuffer(pixbuffer), bitmap(bitmap) { }

        void swap_buffers() override {
            ComPtr<IWICBitmapLock> lock;
            HRESULT hr;
            BYTE *data;
            UINT size;
            hr = bitmap->Lock(nullptr, WICBitmapLockRead, lock.GetAddressOf()); 
            hr = lock->GetDataPointer(&size, &data);

            BTK_ASSERT(size == pixbuffer->pitch() * pixbuffer->height());
            Btk_memcpy(pixbuffer->pixels(), data, size);
        }
    public:
        PixBuffer                    *pixbuffer; //< Pixels buffer write to
        ComPtr<IWICBitmap>               bitmap;
};

#if   defined(BTK_DIRECT2D_EXTENSION1)
class D2DDeviceContext final : public D2DRenderTarget {
    public:
        D2DDeviceContext(PaintDevice *dev, IDXGISwapChain *sw, ID2D1DeviceContext *ctxt) :
            D2DRenderTarget(dev, ctxt), d3d_swapchain(sw), d2d_context(ctxt) { }

        void swap_buffers() override {
            d3d_swapchain->Present(0, 0);
        }
    public:
        ComPtr<IDXGISwapChain>         d3d_swapchain;
        ComPtr<ID2D1DeviceContext>     d2d_context;
};
#endif
/**
 * @brief Interface exposed to 
 * 
 */
class D2DPaintDevice    : public WindowDevice {
    public:
        D2DPaintDevice();
        ~D2DPaintDevice();

        auto paint_context() -> PaintContext * override;
        auto query_value(PaintDeviceValue v, void *out) -> bool override;

        // Inherit from WindowDevice
        void set_dpi(float xdpi, float ydpi) override;
        void resize(int w, int h) override;

    private:
        Direct2DInitializer      initializer;
        WincodecInitializer     winitializer;
    protected:
        // Factorys of D2D1
        ComPtr<ID2D1Factory>     factory;

#if    defined(BTK_DIRECT2D_EXTENSION1)
        ComPtr<ID2D1Factory1>    factory1;
#endif

        ComPtr<ID2D1RenderTarget> target;
        Ref<D2DRenderTarget>     context;
};

/**
 * @brief Render to software pixel buffer
 * 
 */
class D2DWicDevice final : public D2DPaintDevice {
    public:
        D2DWicDevice(PixBuffer *pixbuffer);

        auto paint_context() -> PaintContext *;
    private:
        PixBuffer         *pixbuffer;
        ComPtr<IWICBitmap> bitmap;
};

/**
 * @brief ID2D1HwndRenderTarget
 * 
 */
class D2DHwndDevice final   : public D2DPaintDevice {
    public:
        D2DHwndDevice(HWND hwnd);
    private:
        void resize(int w, int h) override;

        HWND                      hwnd = nullptr;
};

#if    defined(BTK_DIRECT2D_EXTENSION1)
/**
 * @brief Direct2D 1.1 
 * 
 */
class D2DHwndDeviceEx final  : public D2DPaintDevice {
    public:
        D2DHwndDeviceEx(HWND hwnd);

        auto paint_context() -> PaintContext *;
        void resize(int w, int h) override;

        HWND                 hwnd = nullptr;

        ComPtr<ID3D11Device> d3d_device;
        ComPtr<ID3D11DeviceContext> d3d_context;
        ComPtr<IDXGISwapChain> d3d_swapchain;
        ComPtr<ID3D11RenderTargetView> d3d_rendertarget;
        ComPtr<ID3D11Texture2D> d3d_backbuffer;

        ComPtr<ID2D1Device> d2d_device;
        ComPtr<ID2D1DeviceContext> d2d_context;
};
#endif
/**
 * @brief ID2D1PathGeometry wrapper , This lifetime decided by the Factorys
 * 
 */
class D2DPath         final : public PaintResource, public PainterPathSink {
    public:
        BTK_MAKE_PAINT_RESOURCE

        // From PainterPathSink
        void open() override;
        void close() override;

        void move_to(float x, float y) override;
        void line_to(float x, float y) override;
        // void quad_to(float x1, float y1, float x2, float y2) override;
        void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) override;
        // void arc_to(float x1, float y1, float x2, float y2, float radius) override;

        void close_path() override;
        void set_winding(PathWinding winding) override;

        auto signal_destroyed() -> Signal<void()> & override { return signal; }

        ComPtr<ID2D1PathGeometry> path;
        ComPtr<ID2D1GeometrySink> sink;
        Signal<void()>            signal;

        bool  is_figure_open = false;
        float last_x = 0; //< Last Pen x
        float last_y = 0; //< Last Pen y
};
/**
 * @brief D2D1Brush wrapper , This lifetime decided by the RenderTarget
 * 
 */
class D2DBrush      final  : public PaintResource {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DBrush(D2DRenderTarget *target, ID2D1Brush *brush) : target(target), brush(brush) { }

        auto signal_destroyed() -> Signal<void()> & override {
            return target->signal_destroyed();
        }

        D2DRenderTarget   *target; //< Target belong
        ComPtr<ID2D1Brush> brush;
};
/**
 * @brief D2D1StrokeStyle wrapper , This lifetime decided by the Factorys
 * 
 */
class D2DPen         final : public PaintResource {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DPen(ID2D1StrokeStyle *pen) : pen(pen) { }

        auto signal_destroyed() -> Signal<void()> & override {
            return signal;
        }

        ComPtr<ID2D1StrokeStyle> pen;
        Signal<void()>           signal;
};
/**
 * @brief ID2D1Bitmap wrapper , This lifetime decided by the RenderTarget
 * 
 */
class D2DTexture      final : public AbstractTexture {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DTexture(D2DRenderTarget *target, ID2D1Bitmap *bitmap) : target(target), bitmap(bitmap) {
            // In most way, HwndTarget in Windows 8.1 or later use DeviceContext, so we can use it
#if     defined(BTK_DIRECT2D_EXTENSION1)
            this->bitmap.As(&bitmap1);
#endif
        }

        auto signal_destroyed() -> Signal<void()> & override {
            return target->signal_destroyed();
        }

        // Inherit from PaintDevice
        bool query_value(PaintDeviceValue value, void *out) override;
        auto paint_context() -> PaintContext * override;

        void update(const Rect *rect, cpointer_t data, int pitch) override;
        void set_interpolation_mode(InterpolationMode mode) override;

        static constexpr uint32_t Magic = 0x11451415;

        uint32_t           magic = Magic;
        D2DRenderTarget   *target; //< Target belong
        ComPtr<ID2D1Bitmap> bitmap;
        
#if     defined(BTK_DIRECT2D_EXTENSION1)
        ComPtr<ID2D1Bitmap1> bitmap1;
#endif

        D2D1_BITMAP_INTERPOLATION_MODE mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
};

auto D2DRectFrom(const FRect &r) -> D2D1_RECT_F {
    return D2D1::RectF(
        r.x, 
        r.y,
        r.x + r.w,
        r.y + r.h
    );
}
auto D2DPixFormatFrom(PixFormat b) -> D2D1_PIXEL_FORMAT {
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
auto bounds_from_ellipse(float x, float y, float xr, float yr) -> FRect {
    return FRect(
        x - xr,
        y - yr,
        xr * 2,
        yr * 2
    );
}
auto bounds_from_line(float x1, float y1, float x2, float y2) -> FRect {
    auto r = D2D1::RectF(
        std::min(x1, x2),
        std::min(y1, y2),
        std::max(x1, x2),
        std::max(y1, y2)
    );
    return FRect(
        r.left,
        r.top,
        r.right - r.left,
        r.bottom - r.top
    );
}

D2DRenderTarget::D2DRenderTarget(PaintDevice *dev, ID2D1RenderTarget *t) : paint_device(dev), target(t) {
    HRESULT hr;
    
    hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &solid_brush);
}
D2DRenderTarget::~D2DRenderTarget() {
    signal.emit();
}
auto D2DRenderTarget::signal_destroyed() -> Signal<void()> & {
    return signal;
}

void D2DRenderTarget::begin() {
    target->BeginDraw();
    target->SetTransform(D2D1::Matrix3x2F::Identity());
}
void D2DRenderTarget::end() {
    if (has_scissor) {
        // Drop the scissor
        target->PopAxisAlignedClip();
        has_scissor = false;
    }
    auto hr = target->EndDraw();
    BTK_ASSERT(SUCCEEDED(hr));
}
void D2DRenderTarget::swap_buffers() {
    // No-op
}

void D2DRenderTarget::clear(Brush &brush) {
    if (brush.type() == BrushType::Solid) {
        auto [r, g, b, a] = brush.color();
        target->Clear(D2D1::ColorF(r, g, b, a));
        return;
    }
    // We need use Brush to paint the background
    target->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Prepare the brush
    auto [w, h] = target->GetSize();
    FRect area(0, 0, w, h);

    // Fill the background
    target->FillRectangle(D2DRectFrom(area), get_brush(area));
}

// Draw
bool D2DRenderTarget::draw_path(const PainterPath &path) { 
    auto d2d_path = get_path(path);
    D2D1_RECT_F area;

    d2d_path->GetBounds(nullptr, &area);

    auto brush = get_brush(FRect(area.left, area.top, area.right - area.left, area.bottom - area.top));

    target->DrawGeometry(d2d_path, brush, stroke_width, get_pen());

    return true; 
}
bool D2DRenderTarget::draw_line(float x1, float y1, float x2, float y2) {  
    target->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2), 
        get_brush(bounds_from_line(x1, y1, x2, y2)),
        stroke_width,
        get_pen()
    );
    return true; 
}
bool D2DRenderTarget::draw_rect(float x, float y, float w, float h) {  
    auto rect = FRect(x, y, w, h);
    target->DrawRectangle(D2DRectFrom(rect), get_brush(rect), stroke_width, get_pen());
    return true;
}
bool D2DRenderTarget::draw_rounded_rect(float x, float y, float w, float h, float r) {  
    auto rect = FRect(x, y, w, h);
    target->DrawRoundedRectangle(D2D1::RoundedRect(D2DRectFrom(rect), r, r), get_brush(rect), stroke_width, get_pen());
    return true; 
}
bool D2DRenderTarget::draw_ellipse(float x, float y, float xr, float yr) {  
    target->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(x, y), xr, yr), 
        get_brush(bounds_from_ellipse(x, y, xr, yr)),
        stroke_width,
        get_pen()
    );
    return true;
}
bool D2DRenderTarget::draw_image(AbstractTexture *tex_, const FRect *dst, const FRect *src) {

#if !defined(NDEBUG) && !defined(BTK_NO_RTTI)
    BTK_ASSERT(dynamic_cast<D2DTexture*>(tex_) != nullptr);
#endif

    D2DTexture *tex = static_cast<D2DTexture *>(tex_);

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

    target->DrawBitmap(bitmap, d2d_dst, alpha, tex->mode, d2d_src);

    Btk_stack_delete(d2d_src);
    Btk_stack_delete(d2d_dst);
    return true;
}

// Text
bool D2DRenderTarget::draw_text(Alignment align, Font &font, u8string_view text, float x, float y) { 
    // Try Query The TextFormat
    IDWriteTextFormat *fmt;
    IDWriteFactory *factory;
    if (!font.native_handle(FontHandle::IDWriteTextFormat, &fmt)) {
        BTK_LOG("[D2DRenderTarget::draw_text] This backend currently only supports dwrite font");
        return false;
    }
    if (!font.native_handle(FontHandle::IDWriteFactory, &factory)) {
        BTK_LOG("[D2DRenderTarget::draw_text] This backend currently only supports dwrite font");
        return false;
    }
    auto u16 = text.to_utf16();

    if (align == AlignLeft + AlignTop && brush.type() == BrushType::Solid) {
        // Left Top & brush is solid
        auto [r, g, b, a] = brush.color();
        solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
        solid_brush->SetOpacity(alpha);

        auto client = D2D1::InfiniteRect();
        client.left = x;
        client.top = y;

        target->DrawText(
            reinterpret_cast<const WCHAR*>(u16.c_str()),
            u16.length(),
            fmt,
            &client,
            solid_brush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
        );
        return true;
    }

    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = factory->CreateTextLayout(
        reinterpret_cast<const WCHAR *>(u16.c_str()),
        u16.length(),
        fmt,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        &layout
    );

    if (FAILED(hr)) {
        return false;
    }

    draw_text_layout(align, layout.Get(), x, y);

    return true; 
}
bool D2DRenderTarget::draw_text(Alignment align, const TextLayout &layout       , float x, float y) {  
    IDWriteTextLayout *lay;

    if (!layout.native_handle(TextLayoutHandle::IDWriteTextLayout, &lay)) {
        BTK_LOG("[D2DRenderTarget::draw_text] This backend currently only supports dwrite font");
        return false;
    }

    draw_text_layout(align, lay, x, y);

    return true; 
}
void D2DRenderTarget::draw_text_layout(Alignment align, IDWriteTextLayout *layout, float x, float y) {
    // Translate by align to left/top
    HRESULT hr;
    DWRITE_TEXT_METRICS metrics;
    hr = layout->GetMetrics(&metrics);
    assert(SUCCEEDED(hr));

    if ((align & Alignment::Right) == Alignment::Right) {
        x -= metrics.width;
    }
    if ((align & Alignment::Bottom) == Alignment::Bottom) {
        y -= metrics.height;
    }
    if ((align & Alignment::Center) == Alignment::Center) {
        x -= metrics.width / 2;
    }
    if ((align & Alignment::Middle) == Alignment::Middle) {
        y -= metrics.height / 2;
    }
    if ((align & Alignment::Baseline) == Alignment::Baseline) {
        // Get layout baseline
        DWRITE_LINE_METRICS line_metrics;
        UINT line_count = 0;
        hr = layout->GetLineMetrics(&line_metrics, 1, &line_count);
        assert(SUCCEEDED(hr));

        y -= line_metrics.baseline;
    }

    FRect area(x, y, metrics.width, metrics.height);

    target->DrawTextLayout(
        D2D1::Point2F(x, y),
        layout,
        get_brush(area),
        D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
    );
}

// Fill
bool D2DRenderTarget::fill_path(const PainterPath &path) {  
    auto d2d_path = get_path(path);
    D2D1_RECT_F area;

    d2d_path->GetBounds(nullptr, &area);

    auto brush = get_brush(FRect(area.left, area.top, area.right - area.left, area.bottom - area.top));

    target->FillGeometry(d2d_path, brush);
    return true; 
}
bool D2DRenderTarget::fill_rect(float x, float y, float w, float h) {  
    auto rect = FRect(x, y, w, h);
    target->FillRectangle(D2DRectFrom(rect), get_brush(rect));
    return true;
}
bool D2DRenderTarget::fill_rounded_rect(float x, float y, float w, float h, float r) {  
    auto rect = FRect(x, y, w, h);
    target->FillRoundedRectangle(D2D1::RoundedRect(D2DRectFrom(rect), r, r), get_brush(rect));
    return true; 
}
bool D2DRenderTarget::fill_ellipse(float x, float y, float xr, float yr) {  
    target->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(x, y), xr, yr), 
        get_brush(bounds_from_ellipse(x, y, xr, yr))
    );
    return true; 
}
bool D2DRenderTarget::fill_mask(AbstractTexture *mask_, const FRect *dst, const FRect *src) {  
    
#if !defined(NDEBUG) && !defined(BTK_NO_RTTI)
    BTK_ASSERT(dynamic_cast<D2DTexture*>(mask_) != nullptr);
#endif

    D2DTexture *mask = static_cast<D2DTexture *>(mask_);

    D2D1_RECT_F *d2d_src = nullptr;
    D2D1_RECT_F *d2d_dst = nullptr;

    FRect area;

    if (dst) {
        // d2d_dst = static_cast<D2D1_RECT_F*>(_alloca(sizeof(D2D1_RECT_F)));
        d2d_dst = Btk_stack_new(D2D1_RECT_F);
        *d2d_dst = D2D1::RectF(dst->x, dst->y, dst->x + dst->w, dst->y + dst->h);
        area = *dst;
    }
    else {
        auto [w, h] = target->GetSize();
        area.x = 0;
        area.y = 0;
        area.w = w;
        area.h = h;
    }

    if (src) {
        // d2d_src = static_cast<D2D1_RECT_F*>(_alloca(sizeof(D2D1_RECT_F)));
        d2d_src = Btk_stack_new(D2D1_RECT_F);
        *d2d_src = D2D1::RectF(src->x, src->y, src->x + src->w, src->y + src->h);
    }

    target->FillOpacityMask(
        mask->bitmap.Get(),
        get_brush(area),
        D2D1_OPACITY_MASK_CONTENT_GRAPHICS,
        d2d_dst,
        d2d_src
    );

    Btk_stack_delete(d2d_src);
    Btk_stack_delete(d2d_dst);
    return false;
}

auto D2DRenderTarget::create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> AbstractTexture * {
    auto d2d_fmt = D2DPixFormatFrom(fmt);
    ComPtr<ID2D1Bitmap> bitmap;
    auto hr = target->CreateBitmap(
        D2D1::SizeU(w, h),
        D2D1::BitmapProperties(
            d2d_fmt,
            xdpi,
            ydpi
        ),
        bitmap.GetAddressOf()
    );

    return new D2DTexture(this, bitmap.Get());
}
auto D2DRenderTarget::get_brush(const FRect &area) -> ID2D1Brush * {
    auto result = map_brush(brush, area);
    result->SetOpacity(alpha);
    return result;
}
auto D2DRenderTarget::map_brush(Brush &brush, const FRect &area) -> ID2D1Brush * {
    auto type = brush.type();
    // Handle normal brush
    if (type == BrushType::Solid) {
        auto [r, g, b, a] = brush.color();
        solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
        return solid_brush.Get();
    }

    switch (type) {
        case BrushType::LinearGradient : { 
            return map_linear_brush(brush, area);
        }
        case BrushType::RadialGradient : {
            return map_radial_brush(brush, area);
        }
        case BrushType::Bitmap : {
            return map_bitmap_brush(brush, area);
        }
        default : {
            BTK_LOG("[D2DRenderTarget::map_brush] invalid brush type\n");
        }
    }
    return nullptr;
}
auto D2DRenderTarget::map_linear_brush(Brush &brush, const FRect &area) -> ID2D1Brush *{
    auto resource = static_cast<D2DBrush*>(brush.query_device_resource(this));
    auto &info = brush.linear_gradient();

    ID2D1LinearGradientBrush *lgbrush;
    if (resource) {
        lgbrush = static_cast<ID2D1LinearGradientBrush*>(resource->brush.Get());
    }
    else {
        // We need to create one
        auto col = map_gradient(info);

        // Begin create
        ComPtr<ID2D1LinearGradientBrush> out;
        
        HRESULT hr = target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(),
                D2D1::Point2F()
            ),
            col.Get(),
            out.GetAddressOf()
        );

        // Push to cache
        lgbrush = out.Get();

        resource = new D2DBrush(this, out.Get());
        brush.bind_device_resource(this, resource);
    }

    // Configure
    auto start_point = brush.point_to_abs(paint_device, area, info.start_point());
    auto end_point = brush.point_to_abs(paint_device, area, info.end_point());

    lgbrush->SetStartPoint(D2D1::Point2F(start_point.x, start_point.y));
    lgbrush->SetEndPoint(D2D1::Point2F(end_point.x, end_point.y));

    return lgbrush;
}
auto D2DRenderTarget::map_radial_brush(Brush &brush, const FRect &area) -> ID2D1Brush * {
    auto resource = static_cast<D2DBrush*>(brush.query_device_resource(this));
    auto &info = brush.radial_gradient();

    ID2D1RadialGradientBrush *rdbrush;
    if (resource) {
        rdbrush = static_cast<ID2D1RadialGradientBrush*>(resource->brush.Get());
    }
    else {
        // We need to create one
        auto col = map_gradient(info);

        // Begin create
        ComPtr<ID2D1RadialGradientBrush> out;
        
        HRESULT hr = target->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(
                D2D1::Point2F(),
                D2D1::Point2F(),
                0.0f,
                0.0f
            ),
            col.Get(),
            out.GetAddressOf()
        );

        // Push to cache
        rdbrush = out.Get();

        resource = new D2DBrush(this, out.Get());
        brush.bind_device_resource(this, resource);
    }

    // Configure
    auto center = brush.point_to_abs(paint_device, area, info.center_point());
    auto radius = brush.point_to_abs(paint_device, area, {info.radius_x(), info.radius_y()});
    auto offset = info.origin_offset();

    // Translate offset value
    switch (brush.coordinate_mode()) {
        case CoordinateMode::Absolute : {
            // Nothing to do
            break;
        }
        case CoordinateMode::Device : {
            [[fallthrough]];
        }
        case CoordinateMode::Relative : {
            offset.x *= radius.x;
            offset.y *= radius.y;
            break;
        }
    }
    rdbrush->SetCenter(D2D1::Point2F(center.x, center.y));
    rdbrush->SetGradientOriginOffset(D2D1::Point2F(offset.x, offset.y));
    rdbrush->SetRadiusX(radius.x);
    rdbrush->SetRadiusY(radius.y);
    return rdbrush;
}
auto D2DRenderTarget::map_bitmap_brush(Brush &brush, const FRect &area) -> ID2D1Brush * {
    auto rect = brush.rect();
    auto pixbuffer = brush.bitmap();
    auto abs_rect = brush.rect_to_abs(paint_device, area, rect);
    auto resource = static_cast<D2DBrush*>(brush.query_device_resource(this));

    ID2D1BitmapBrush *btbrush;
    if (resource) {
        btbrush = static_cast<ID2D1BitmapBrush*>(resource->brush.Get());
    }
    else {
        ComPtr<ID2D1BitmapBrush> out;
        ComPtr<ID2D1Bitmap>   bitmap;
        HRESULT hr;

        hr = target->CreateBitmap(
            D2D1::SizeU(pixbuffer.width(), pixbuffer.height()),
            pixbuffer.pixels(),
            pixbuffer.pitch(),
            D2D1::BitmapProperties(
                D2DPixFormatFrom(pixbuffer.format())
            ),
            bitmap.GetAddressOf()
        );
        if (FAILED(hr)) {
            BTK_LOG("[D2DRenderTarget::map_bitmap_brush] Failed to create bitmap\n");
            return nullptr;
        }

        hr = target->CreateBitmapBrush(bitmap.Get(), out.GetAddressOf());
        if (FAILED(hr)) {
            BTK_LOG("[D2DRenderTarget::map_bitmap_brush] Failed to create bitmap brush\n");
            return nullptr;
        }

        // Bind to the brush
        resource = new D2DBrush(this, out.Get());
        brush.bind_device_resource(this, resource);

        btbrush = out.Get();
    }
    // Configure brush
    float target_width = abs_rect.w;
    float target_height = abs_rect.h;
    float w_scale = target_width / pixbuffer.width();
    float h_scale = target_height / pixbuffer.height();

    btbrush->SetTransform(
        D2D1::Matrix3x2F::Scale(
            w_scale,
            h_scale
        )
        *
        D2D1::Matrix3x2F::Translation(
            abs_rect.x,
            abs_rect.y
        )
    );

    return btbrush;
}
auto D2DRenderTarget::map_gradient(const Gradient &gradient) -> ComPtr<ID2D1GradientStopCollection> {
    auto &stops = gradient.stops();
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
}
auto D2DRenderTarget::get_pen() -> ID2D1StrokeStyle * {
    if (pen.empty()) {
        return nullptr;
    }
    // Begin convert pen
    // Becaouse that pen is based on Direct2D Factory, so use it as key
    auto resource = static_cast<D2DPen*>(pen.query_device_resource(Direct2D::GetInstance()));
    ID2D1StrokeStyle *stroke_style;

    if (resource) {
        stroke_style = resource->pen.Get();
    }
    else {
        ComPtr<ID2D1StrokeStyle> out;
        D2D1_CAP_STYLE d2d_cap;
        D2D1_LINE_JOIN d2d_join;
        D2D1_DASH_STYLE d2d_style;

        switch (pen.line_cap()) {
            case LineCap::Flat: d2d_cap = D2D1_CAP_STYLE_FLAT; break;
            case LineCap::Round: d2d_cap = D2D1_CAP_STYLE_ROUND; break;
            case LineCap::Square: d2d_cap = D2D1_CAP_STYLE_SQUARE; break;
        }
        switch (pen.line_join()) {
            case LineJoin::Miter: d2d_join = D2D1_LINE_JOIN_MITER; break;
            case LineJoin::Round: d2d_join = D2D1_LINE_JOIN_ROUND; break;
            case LineJoin::Bevel: d2d_join = D2D1_LINE_JOIN_BEVEL; break;
        }
        switch (pen.dash_style()) {
            case DashStyle::Custom : d2d_style = D2D1_DASH_STYLE_CUSTOM; break;
            case DashStyle::Solid : d2d_style = D2D1_DASH_STYLE_SOLID; break;
            case DashStyle::Dash  : d2d_style = D2D1_DASH_STYLE_DASH; break;
            case DashStyle::Dot  : d2d_style = D2D1_DASH_STYLE_DOT; break;
            case DashStyle::DashDot  : d2d_style = D2D1_DASH_STYLE_DASH_DOT; break;
            case DashStyle::DashDotDot  : d2d_style = D2D1_DASH_STYLE_DASH_DOT_DOT; break;
        }

        auto &pattern = pen.dash_pattern();

        HRESULT hr = Direct2D::GetInstance()->CreateStrokeStyle(
            D2D1::StrokeStyleProperties(
                d2d_cap,
                d2d_cap,
                d2d_cap,
                d2d_join,
                pen.miter_limit(),
                d2d_style,
                pen.dash_offset()
            ),
            pattern.data(),
            pattern.size(),
            out.GetAddressOf()
        );

        if (FAILED(hr)) {
            BTK_LOG("[D2DRenderTarget::get_pen] Failed to create ID2D1StrokeStyle\n");
            return nullptr;
        }

        // Bind
        resource = new D2DPen(out.Get());
        pen.bind_device_resource(Direct2D::GetInstance(), resource);

        stroke_style = out.Get();
    }

    return stroke_style;
}
auto D2DRenderTarget::get_path(const PainterPath &_path) -> ID2D1PathGeometry * {
    auto &path = const_cast<PainterPath &>(_path);
    // As same as pen
    auto resource = static_cast<D2DPath*>(path.query_device_resource(Direct2D::GetInstance()));
    if (!resource) {
        resource = new D2DPath();
        // Stream into d2dpath
        path.stream(resource);

        path.bind_device_resource(Direct2D::GetInstance(), resource);
    }
    return resource->path.Get();
}

bool D2DRenderTarget::set_state(PaintContextState state, const void *data) {
    switch (state) {
        case PaintContextState::Transform : {
            D2D1::Matrix3x2F mat;
            if (data == nullptr) {
                // To Identity
                mat = D2D1::Matrix3x2F::Identity();
            }
            else {
                static_assert(sizeof(D2D1::Matrix3x2F) == sizeof(FMatrix));
                mat = *static_cast<const D2D1::Matrix3x2F*>(data);
            }
            target->SetTransform(mat);
            return true;
        }
        case PaintContextState::StrokeWidth : {
            stroke_width = *static_cast<const float*>(data);
            return true;
        }
        case PaintContextState::Alpha : {
            alpha = *static_cast<const float*>(data);
            return true;
        }
        case PaintContextState::Antialias : {
            target->SetAntialiasMode(
                *static_cast<const bool*>(data) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED
            );
            return true;
        }
        case PaintContextState::Pen : {
            pen = *static_cast<const Pen*>(data);
            return true;
        }
        case PaintContextState::Brush : {
            brush = *static_cast<const Brush*>(data);
            return true;
        }
        case PaintContextState::Scissor : {
            if (has_scissor) {
                target->PopAxisAlignedClip();
            }
            // Check args
            if (data) {
                has_scissor = true;
                // Has scissor
                auto scissor = static_cast<const PaintScissor*>(data);
                D2D1::Matrix3x2F prev_mat;

                // Apply scissor transform
                target->GetTransform(&prev_mat);
                target->SetTransform(reinterpret_cast<const D2D1::Matrix3x2F&>(scissor->matrix));

                target->PushAxisAlignedClip(
                    D2DRectFrom(scissor->rect),
                    target->GetAntialiasMode()
                );

                // Restore
                target->SetTransform(prev_mat);
            }
            else {
                has_scissor = false;
            }
            return true;
        }
        default : {
            BTK_LOG("[D2DRenderTarget::set_state] Unknown state opcode %d\n", int(state));
        }
    }
    return false;
}

// D2DBitmapTarget


// D2DTexture Implementation
bool D2DTexture::query_value(PaintDeviceValue value, void *out) {
    switch (value) {
        case PaintDeviceValue::PixelSize : {
            auto fs = static_cast<Size*>(out);
            auto [w, h] = bitmap->GetPixelSize();
            fs->w = w;
            fs->h = h;
            break;
        }
        case PaintDeviceValue::LogicalSize : {
            auto fs = static_cast<FSize*>(out);
            auto [w, h] = bitmap->GetSize();
            fs->w = w;
            fs->h = h;
            break;
        }
        case PaintDeviceValue::Dpi : {
            auto fp = static_cast<FPoint*>(out);
            bitmap->GetDpi(&fp->x, &fp->y);
            break;
        }
        default : return false;
    }
    return true;
}
auto D2DTexture::paint_context() -> PaintContext * {
    ComPtr<ID2D1BitmapRenderTarget> bitmap_target;
    auto hr = target->target->CreateCompatibleRenderTarget(
        bitmap->GetSize(),
        bitmap->GetPixelSize(),
        bitmap->GetPixelFormat(),
        bitmap_target.GetAddressOf()
    );
    if (FAILED(hr)) {
        return nullptr;
    }
    return new D2DBitmapTarget(this, bitmap_target.Get(), bitmap.Get());
}
void D2DTexture::update(const Rect *where, cpointer_t data, int pitch) {
    D2D1_RECT_U *rect = nullptr;
    if (where) {
        rect = (D2D1_RECT_U*) _malloca(sizeof(D2D1_RECT_F));
        rect->left = where->x;
        rect->top = where->y;
        rect->right = where->x + where->w;
        rect->bottom = where->y + where->h;
    }
    HRESULT hr = bitmap->CopyFromMemory(rect, data, pitch);
    if (FAILED(hr)) {
        BTK_LOG("Failed to update texture: ");
    }
    _freea(rect);
}
void D2DTexture::set_interpolation_mode(InterpolationMode m) {
    switch (m) {
        case InterpolationMode::Nearest : mode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR; break;
        case InterpolationMode::Linear : mode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR; break;
    }
}

// D2DPath
void D2DPath::open() {
    HRESULT hr;

    hr = Direct2D::GetInstance()->CreatePathGeometry(path.GetAddressOf());
    if (FAILED(hr)) {
        BTK_LOG("[D2DPath::open] Failed to create path geometry\n");
    }
    hr = path->Open(sink.GetAddressOf());
    if (FAILED(hr)) {
        BTK_LOG("[D2DPath::open] Failed to open path geometry sink\n");
    }
}
void D2DPath::close() {
    if (is_figure_open) {
        is_figure_open = false;
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    sink->Close();
}
void D2DPath::move_to(float x, float y) {
    // Start a new figure if not open
    if (is_figure_open) {
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    sink->BeginFigure(
        D2D1::Point2F(x, y),
        D2D1_FIGURE_BEGIN_FILLED
    );
    is_figure_open = true;
    last_x = x;
    last_x = y;
}
void D2DPath::line_to(float x, float y) {
    sink->AddLine(D2D1::Point2F(x, y));
    last_x = x;
    last_x = y;
}
// void D2DPath::quad_to(float x1, float y1, float x2, float y2) {
//     sink->AddQuadraticBezier(
//         D2D1::QuadraticBezierSegment(
//             D2D1::Point2F(x1, y1),
//             D2D1::Point2F(x2, y2)
//         )
//     );
//     last_x = x2;
//     last_x = y2;
// }
void D2DPath::bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) {
    sink->AddBezier(
        D2D1::BezierSegment(
            D2D1::Point2F(x1, y1),
            D2D1::Point2F(x2, y2),
            D2D1::Point2F(x3, y3)
        )
    );
    last_x = x3;
    last_x = y3;
}
// void D2DPath::arc_to(float x1, float y1, float x2, float y2, float radius) {
//     BTK_ONCE(BTK_LOG("NanoVG impl for arc_to\n"));
//     fallback_painter_arc_to(this, last_x, last_y, x1, y1, x2, y2, radius);
// }
void D2DPath::close_path() {
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    is_figure_open = false;
}
void D2DPath::set_winding(PathWinding winding) {
    D2D1_FILL_MODE mode;
    switch (winding) {
        case PathWinding::Solid : mode = D2D1_FILL_MODE_WINDING; break;
        case PathWinding::Hole : mode = D2D1_FILL_MODE_ALTERNATE;   break;
    }
    sink->SetFillMode(mode);
}

// D2DPaintDevice Implementation
D2DPaintDevice::D2DPaintDevice() {
    factory = Direct2D::GetInstance();

    // Direct2D 1.1
#if    defined(BTK_DIRECT2D_EXTENSION1)
    factory.As(&factory1);
#endif

}
D2DPaintDevice::~D2DPaintDevice() {

}
auto D2DPaintDevice::paint_context() -> PaintContext * {
    if (!context) {
        // No context, prepare it
        context.reset(new D2DRenderTarget(this, target.Get()));
    }
    return context.get();
}
bool D2DPaintDevice::query_value(PaintDeviceValue value, void *out) {
    switch (value) {
        case PaintDeviceValue::PixelSize : {
            auto fs = static_cast<Size*>(out);
            auto [w, h] = target->GetPixelSize();
            fs->w = w;
            fs->h = h;
            break;
        }
        case PaintDeviceValue::LogicalSize : {
            auto fs = static_cast<FSize*>(out);
            auto [w, h] = target->GetSize();
            fs->w = w;
            fs->h = h;
            break;
        }
        case PaintDeviceValue::Dpi : {
            auto fp = static_cast<FPoint*>(out);
            target->GetDpi(&fp->x, &fp->y);
            break;
        }
        default : return false;
    }
    return true;
}

void D2DPaintDevice::resize(int w, int h) {
    // No-op
}
void D2DPaintDevice::set_dpi(FLOAT xdpi, FLOAT ydpi) {
    target->SetDpi(xdpi, ydpi);
}

// D2D Wic
D2DWicDevice::D2DWicDevice(PixBuffer *buffer) : pixbuffer(buffer) {
    // CUrrently unsupported now
    if (buffer->format() != PixFormat::RGBA32) {
        BTK_THROW(std::runtime_error("PixBuffer must have RGBA32 format"));
    }

    HRESULT hr;

    hr = Wincodec::GetInstance()->CreateBitmapFromMemory(
        buffer->width(),
        buffer->height(),
        GUID_WICPixelFormat32bppRGB,
        buffer->pitch(),
        buffer->pitch() * buffer->height(),
        buffer->pixels<BYTE>(),
        bitmap.GetAddressOf()
    );

    hr = Direct2D::GetInstance()->CreateWicBitmapRenderTarget(
        bitmap.Get(),
        D2D1::RenderTargetProperties(),
        target.GetAddressOf()
    );
}
auto D2DWicDevice::paint_context() -> PaintContext* {
    if (!context) {
        context.reset(new D2DWicBitmapTarget(this, target.Get(), pixbuffer, bitmap.Get()));
    }
    return context.get();
} 


D2DHwndDevice::D2DHwndDevice(HWND h) {
    hwnd = h;
    auto hr = Direct2D::GetInstance()->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED
            )
        ),
        D2D1::HwndRenderTargetProperties(hwnd),
        reinterpret_cast<ID2D1HwndRenderTarget**>(target.GetAddressOf())
    );
    if (FAILED(hr)) {
        ::abort();
    }
}
void D2DHwndDevice::resize(int w, int h) {
    auto hw_target = static_cast<ID2D1HwndRenderTarget*>(target.Get());
    // Is HWND client
    RECT rect;
    GetClientRect(hwnd, &rect);
    hw_target->Resize(D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top));
}

#if  defined(BTK_DIRECT2D_EXTENSION1)
// FIXME : DPI Incorrent here
D2DHwndDeviceEx::D2DHwndDeviceEx(HWND hwnd) : hwnd(hwnd) {
    // Create d3d device & swapchains

    // Prepare features level
    HRESULT hr;

    DXGI_SWAP_CHAIN_DESC desc = {};
    RECT rect;
    HDC  dc;

    dc = GetDC(hwnd);
    GetClientRect(hwnd, &rect);
    
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.Width = rect.right - rect.left;
    desc.BufferDesc.Height = rect.bottom - rect.top;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.RefreshRate.Numerator = GetDeviceCaps(dc, VREFRESH);


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

    ReleaseDC(hwnd, dc);


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

    ComPtr<IDXGIDevice> dxgi_device;
    hr = d3d_device.As(&dxgi_device);
    if (FAILED(hr)) {
        BTK_THROW(std::runtime_error("Failed to Get DXGI Surface\n"));
    }

    hr = factory1->CreateDevice(dxgi_device.Get(), d2d_device.GetAddressOf());
    if (FAILED(hr)) {
        BTK_THROW(std::runtime_error("Failed to Create D2D Device\n"));
    }

    hr = d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_context.GetAddressOf());
    if (FAILED(hr)) {
        BTK_THROW(std::runtime_error("Failed to Create D2D DeviceContext\n"));
    }

    // Resize the buffer
    resize(rect.right - rect.left, rect.bottom - rect.top);
}
void D2DHwndDeviceEx::resize(int w, int h) {
    FLOAT xdpi, ydpi;
    d2d_context->GetDpi(&xdpi, &ydpi);

    RECT rect;
    GetClientRect(hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    // Unbind from old target
    d2d_context->SetTarget(nullptr);
    d3d_backbuffer.Reset();

    ComPtr<IDXGISurface> surface;
    ComPtr<ID2D1Bitmap1> bitmap;
    HRESULT hr = d3d_swapchain->ResizeBuffers(0, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    hr = d3d_swapchain->GetBuffer(0, IID_PPV_ARGS(&surface));
    hr = d2d_context->CreateBitmapFromDxgiSurface(
        surface.Get(),
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            xdpi,
            ydpi
        ),
        bitmap.GetAddressOf()
    );
    d2d_context->SetTarget(bitmap.Get());
}
auto D2DHwndDeviceEx::paint_context() -> PaintContext * {
    if 	(!context) {
        context.reset(new D2DDeviceContext(this, d3d_swapchain.Get(), d2d_context.Get()));
    }
    return context.get();
}

#endif


// Register Function
extern "C" void __BtkPlatform_D2D_Init() {
    RegisterPaintDevice<PixBuffer>([](PixBuffer *buf) -> PaintDevice * {
        return new D2DWicDevice(buf);
    });
    RegisterPaintDevice<AbstractWindow>([](AbstractWindow *win) -> PaintDevice * {
        HWND hwnd;
        if (win->query_value(AbstractWindow::Hwnd, &hwnd)) {
            return new D2DHwndDevice(hwnd);
        }
        return nullptr;
    });
    RegisterPaintDevice<HWND>([](HWND hwnd) -> PaintDevice * {
        return new D2DHwndDevice(hwnd);
    });
}

BTK_PRIV_END