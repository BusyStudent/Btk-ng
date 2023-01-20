#include "build.hpp"
#include "common/win32/direct2d.hpp"

#include <Btk/detail/reference.hpp>
#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/painter.hpp>
#include <Btk/object.hpp>
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
using Win32::Direct2DInitializer;
using Win32::Direct2D;

using Microsoft::WRL::ComPtr;

class D2DDeviceContext;
class D2DRenderTarget;

// Bind windows message to the device context
void  RegisterD2DRenderTarget(HWND handle, D2DRenderTarget *target);
void  UnregisterD2DRenderTarget(HWND handle);

/**
 * @brief Base D2D1RenderTarget wrapper class
 * 
 */
class D2DRenderTarget  : public PaintContext {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DRenderTarget(ID2D1RenderTarget *tar);
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
        bool draw_path(PainterPath &path, Brush &brush, float stroke_width, Pen &pen) override;
        bool draw_line(FPoint from, FPoint to, Brush &brush, float stroke_width, Pen &pen) override;
        bool draw_rect(FRect rect, Brush &brush, float stroke_width, Pen &pen) override;
        bool draw_rounded_rect(FRect rect, float r, Brush &brush, float stroke_width, Pen &pen) override;
        bool draw_ellipse(FPoint center, float xr, float yr, Brush &brush, float stroke_width, Pen &pen) override;
        bool draw_image(AbstractTexture *image, const FRect *dst, const FRect *src) override;

        // Text
        bool draw_text(Alignment, Font &font, u8string_view text, float x, float y, Brush &) override;
        bool draw_text(Alignment, TextLayout &layout            , float x, float y, Brush &) override;

        // Fill
        bool fill_path(PainterPath &path, Brush &brush) override;
        bool fill_rect(FRect rect, Brush &brush) override;
        bool fill_rounded_rect(FRect rect, float r, Brush &brush) override;
        bool fill_ellipse(FPoint center, float xr, float yr, Brush &brush) override;
        bool fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) override;

        // Texture
        auto create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> AbstractTexture * override;;

        bool set_state(PaintContextState state, const void *data) override;
        bool has_feature(PaintContextFeature feature) override;
        bool native_handle(PaintContextHandle, void *out) override { return false; }
        

        auto brush_of(Brush &, const FRect &area) -> ID2D1Brush *;
        auto path_of(PainterPath &) -> ID2D1PathGeometry *;
        auto pen_of(Pen &)   -> ID2D1StrokeStyle *;

        virtual void on_resize(UINT width, UINT height);
        virtual void on_dpi_changed(FLOAT xdpi, FLOAT ydpi);
    private:
        ComPtr<ID2D1RenderTarget>    target;
        Signal<void()>               signal; //< Signal when destroyed & losted
        ComPtr<ID2D1SolidColorBrush> solid_brush; //< Color Brush
        float                        alpha = 1.0f; //< Global Alpha
};
/**
 * @brief Interface exposed to 
 * 
 */
class D2DPaintDevice    : public PaintDevice {
    public:
        D2DPaintDevice();
        D2DPaintDevice(HWND hwnd);
        D2DPaintDevice(PixBuffer *buffer);
        ~D2DPaintDevice();

        auto paint_context() -> PaintContext * override;
        auto query_value(PaintDeviceValue v, void *out) -> bool override;
    private:
        Direct2DInitializer      initializer;
        ComPtr<ID2D1RenderTarget> target;
        Ref<D2DRenderTarget>     context;

        HWND                      hwnd = nullptr;
};
/**
 * @brief ID2D1PathGeometry wrapper , This lifetime decided by the Factorys
 * 
 */
class D2DPath            : public PaintResource, public PainterPathSink {
    public:
        BTK_MAKE_PAINT_RESOURCE

        auto signal_destroyed() -> Signal<void()> & override { return signal; }

        ComPtr<ID2D1PathGeometry> path;
        Signal<void()>            signal;
};
/**
 * @brief D2D1Brush wrapper , This lifetime decided by the RenderTarget
 * 
 */
class D2DBrush          : public PaintResource {
    public:
        BTK_MAKE_PAINT_RESOURCE

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
class D2DPen            : public PaintResource {
    public:
        BTK_MAKE_PAINT_RESOURCE

        auto signal_destroyed() -> Signal<void()> & override;

        ComPtr<ID2D1StrokeStyle> pen;
        Signal<void()>           signal;
};
/**
 * @brief ID2D1Bitmap wrapper , This lifetime decided by the RenderTarget
 * 
 */
class D2DTexture         : public AbstractTexture {
    public:
        BTK_MAKE_PAINT_RESOURCE

        D2DTexture(D2DRenderTarget *target, ID2D1Bitmap *bitmap) : target(target), bitmap(bitmap) {}

        auto signal_destroyed() -> Signal<void()> & override {
            return target->signal_destroyed();
        }

        // Inherit from PaintDevice
        bool query_value(PaintDeviceValue value, void *out) override;
        auto paint_context() -> PaintContext * override { return nullptr; }

        void update(const Rect *rect, cpointer_t data, int pitch) override;

        static constexpr uint32_t Magic = 0x11451415;

        uint32_t           magic = Magic;
        D2DRenderTarget   *target; //< Target belong
        ComPtr<ID2D1Bitmap> bitmap;
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
        xr,
        yr
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

D2DRenderTarget::D2DRenderTarget(ID2D1RenderTarget *t) : target(t) {
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
    target->FillRectangle(D2DRectFrom(area), brush_of(brush, area));
}

// Draw
bool D2DRenderTarget::draw_path(PainterPath &path, Brush &brush, float stroke_width, Pen &pen) { 
    return true; 
}
bool D2DRenderTarget::draw_line(FPoint from, FPoint to, Brush &brush, float stroke_width, Pen &pen) {  
    target->DrawLine(D2D1::Point2F(
        from.x, from.y), D2D1::Point2F(to.x, to.y), 
        brush_of(brush, bounds_from_line(from.x, from.y, to.x, to.y)),
        stroke_width,
        pen_of(pen)
    );
    return true; 
}
bool D2DRenderTarget::draw_rect(FRect rect, Brush &brush, float stroke_width, Pen &pen) {  
    target->DrawRectangle(D2DRectFrom(rect), brush_of(brush, rect), stroke_width, pen_of(pen));
    return true;
}
bool D2DRenderTarget::draw_rounded_rect(FRect rect, float r, Brush &brush, float stroke_width, Pen &pen) {  
    target->DrawRoundedRectangle(D2D1::RoundedRect(D2DRectFrom(rect), r, r), brush_of(brush, rect), stroke_width, pen_of(pen));
    return true; 
}
bool D2DRenderTarget::draw_ellipse(FPoint center, float xr, float yr, Brush &brush, float stroke_width, Pen &pen) {  
    target->DrawEllipse(
        D2D1::Ellipse(D2D1::Point2F(center.x, center.y), xr, yr), 
        brush_of(brush, bounds_from_ellipse(center.x, center.y, xr, yr)),
        stroke_width,
        pen_of(pen)
    );
    return true;
}
bool D2DRenderTarget::draw_image(AbstractTexture *tex_, const FRect *dst, const FRect *src) {  
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

    target->DrawBitmap(bitmap, d2d_dst, alpha, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, d2d_src);

    return true;
}

// Text
bool D2DRenderTarget::draw_text(Alignment, Font &font, u8string_view text, float x, float y, Brush &) {  
    return true; 
}
bool D2DRenderTarget::draw_text(Alignment, TextLayout &layout            , float x, float y, Brush &) {  
    return true; 
}

// Fill
bool D2DRenderTarget::fill_path(PainterPath &path, Brush &brush) {  
    return true; 
}
bool D2DRenderTarget::fill_rect(FRect rect, Brush &brush) {  
    target->FillRectangle(D2DRectFrom(rect), brush_of(brush, rect));
    return true;
}
bool D2DRenderTarget::fill_rounded_rect(FRect rect, float r, Brush &brush) {  
    target->FillRoundedRectangle(D2D1::RoundedRect(D2DRectFrom(rect), r, r), brush_of(brush, rect));
    return true; 
}
bool D2DRenderTarget::fill_ellipse(FPoint center, float xr, float yr, Brush &brush) {  
    target->FillEllipse(
        D2D1::Ellipse(D2D1::Point2F(center.x, center.y), xr, yr), 
        brush_of(brush, bounds_from_ellipse(center.x, center.y, xr, yr))
    );
    return true; 
}
bool D2DRenderTarget::fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) {  
    return true; 
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
auto D2DRenderTarget::brush_of(Brush &brush, const FRect &area) -> ID2D1Brush * {
    auto type = brush.type();
    switch (type) {
        case BrushType::Solid : {
            auto [r, g, b, a] = brush.color();
            solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
            return solid_brush.Get();
        }
    }
    return solid_brush.Get();
}
auto D2DRenderTarget::pen_of(Pen &) -> ID2D1StrokeStyle * {
    return nullptr;
}

bool D2DRenderTarget::set_state(PaintContextState state, const void *data) {
    return false;
}
bool D2DRenderTarget::has_feature(PaintContextFeature feature) {
    return false;
}


void D2DRenderTarget::on_resize(UINT width, UINT height) {
    ComPtr<ID2D1HwndRenderTarget> hw_target;
    if (SUCCEEDED(target.As(&hw_target))) {
        hw_target->Resize(D2D1::SizeU(width, height));
    }
}
void D2DRenderTarget::on_dpi_changed(FLOAT xdpi, FLOAT ydpi) {
    target->SetDpi(xdpi, ydpi);
}


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
            auto fs = static_cast<Size*>(out);
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

// D2DPaintDevice Implementation
D2DPaintDevice::D2DPaintDevice() {

}
D2DPaintDevice::D2DPaintDevice(HWND h) {
    hwnd = h;
    auto hr = Direct2D::GetInstance()->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd),
        reinterpret_cast<ID2D1HwndRenderTarget**>(target.GetAddressOf())
    );
    if (FAILED(hr)) {
        ::abort();
    }
}
D2DPaintDevice::D2DPaintDevice(PixBuffer *buffer) {
    // CUrrently unsupported now
    ::abort();
}
D2DPaintDevice::~D2DPaintDevice() {
    if (context && hwnd) {
        // Context created & Is Window Target
        UnregisterD2DRenderTarget(hwnd);
    }
}
auto D2DPaintDevice::paint_context() -> PaintContext * {
    if (!context) {
        // No context, prepare it
        context.reset(new D2DRenderTarget(target.Get()));

        if (hwnd) {
            RegisterD2DRenderTarget(hwnd, context.get());
        }
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
            auto fs = static_cast<Size*>(out);
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

// Impl Message Introp for Win32 by HOOK

static std::unordered_map<HWND, D2DRenderTarget*> hwnds_map;
static HHOOK                                      hook;

LRESULT CALLBACK D2DMessageHook(int code, WPARAM wp, LPARAM lp) {
    CWPSTRUCT *window = reinterpret_cast<CWPSTRUCT*>(lp);

    switch (window->message) {
        case WM_SIZE : {
            auto iter = hwnds_map.find(window->hwnd);
            if (iter != hwnds_map.end()) {
                RECT rect;
                GetClientRect(window->hwnd, &rect);
                iter->second->on_resize(rect.right - rect.left, rect.bottom - rect.top);
            }
            break;
        }
        case WM_DPICHANGED : {
            auto iter = hwnds_map.find(window->hwnd);
            if (iter != hwnds_map.end()) {
                DWORD dpi = HIWORD(wp);
                iter->second->on_dpi_changed(dpi, dpi);
            }
            break;
        }
    }

    return CallNextHookEx(hook, code, wp, lp);
}

void  RegisterD2DRenderTarget(HWND handle, D2DRenderTarget *target) {
    if (hwnds_map.size() == 0) {
        // Register
        hook = SetWindowsHookEx(
            WH_CALLWNDPROC,
            D2DMessageHook,
            nullptr,
            GetCurrentThreadId()
        );
    }
    hwnds_map[handle] = target;
}
void  UnregisterD2DRenderTarget(HWND handle) {
    hwnds_map.erase(handle);
    if (hwnds_map.size() == 0) {
        // Unregister
        UnhookWindowsHookEx(hook);
    }
}



// Register Function
extern "C" void __BtkPlatform_D2D_Init() {
    RegisterPaintDevice<PixBuffer>([](PixBuffer *buf) -> PaintDevice * {
        return new D2DPaintDevice(buf);
    });
    RegisterPaintDevice<AbstractWindow>([](AbstractWindow *win) -> PaintDevice * {
        HWND hwnd;
        if (win->query_value(AbstractWindow::Hwnd, &hwnd)) {
            return new D2DPaintDevice(hwnd);
        }
        return nullptr;
    });
    RegisterPaintDevice<HWND>([](HWND hwnd) -> PaintDevice * {
        return new D2DPaintDevice(hwnd);
    });
}

BTK_PRIV_END