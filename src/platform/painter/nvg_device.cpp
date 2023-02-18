#include "build.hpp"

#include <Btk/detail/reference.hpp>
#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/opengl/opengl.hpp>
#include <Btk/painter.hpp>

extern "C" {
    #define NANOVG_GLES3_IMPLEMENTATION
    #include "libs/nanovg.hpp"
    #include "libs/nanovg.cpp"
    #include "libs/nanovg_gl.hpp"
}

#define BTK_MAKE_PAINT_RESOURCE      \
    public:                          \
        uint32_t _refcount = 0;      \
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

class NanoVGContext;
class NanoVGWindowDevice final : public WindowDevice {
    public:
        NanoVGWindowDevice(AbstractWindow *);
        ~NanoVGWindowDevice();

        auto paint_context() -> PaintContext *;
        bool query_value(PaintDeviceValue value, void *out);

        // We didnot need it
        void set_dpi(float, float) override { }
        void resize(int, int) override { }
    private:
        AbstractWindow *window = nullptr;
        GLContext      *glctxt = nullptr;
        NanoVGContext *nvgctxt = nullptr;
    friend class NanoVGContext;
};

class NanoVGContext final : public PaintContext, PainterPathSink, OpenGLES3Function {
    public:
        NanoVGContext(NanoVGWindowDevice *device);
        ~NanoVGContext();

        GLContext *gl_context() const { return glctxt; }
        NVGcontext *nvg_context() const { return nvgctxt; }

        // PaintResource
        void ref() { }
        void unref() { }
        auto signal_destroyed() -> Signal<void()> & { return signal; }
        auto signal_text_cache_destroyed() -> Signal<void()> & { return signal_txt_cache; }

        // Inherit from GraphicsContext
        void begin() override;
        void end() override;
        void swap_buffers() override;

        // Inherit from PaintContext
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

        // State
        bool set_state(PaintContextState state, const void *what) override;

        // Extra
        bool has_feature(PaintContextFeature f) override { return true; }
        bool native_handle(PaintContextHandle h, void *out) override;

        // Texture
        auto create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> AbstractTexture * override;

        // Vector Graphics Sink
        void open() override;
        void close() override;
        void move_to(float x, float y) override;
        void line_to(float x, float y) override;
        void quad_to(float x1, float y1, float x2, float y2) override;
        void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) override;
        void arc_to(float x1, float y1, float x2, float y2, float radius) override;
        void close_path() override;
        void set_winding(PathWinding winding) override;

        auto current_transform() -> FMatrix *;
    private:
        void apply_brush(float x, float y, float w, float h);
        void apply_brush(const FRect &rect);

        NanoVGWindowDevice *device;
        GLContext *glctxt;
        NVGcontext *nvgctxt;

        Signal<void()> signal; //< Signal for cleanup resource
        Signal<void()> signal_txt_cache; //< Signal for cleanup text cache

        // Brush
        bool need_apply_brush = false;
        Brush brush;

        // Sink
        PathWinding winding;

        // Texture
        Size max_texture_size;

        // TextCache
        int numof_cache = 0;
        int max_cache_size = 100;
    friend class NanoVGTexture;
    friend class NanoVGTextCache;
};
class NanoVGTexture final : public AbstractTexture {
    public:
        BTK_MAKE_PAINT_RESOURCE        

        NanoVGTexture(NanoVGContext *ctxt, int id) : ctxt(ctxt), id(id) { }
        ~NanoVGTexture();

        // Inhertied from PaintResource
        auto signal_destroyed() -> Signal<void()> & override {
            return ctxt->signal_destroyed();
        }

        // Inhertied from PaintDevice
        auto paint_context() -> PaintContext * override {
            return nullptr;
        }
        bool query_value(PaintDeviceValue v, void *out) override;

        void update(const Rect *area, cpointer_t ptr, int pitch) override;
        void set_interpolation_mode(InterpolationMode mode) override;
    private:
        NanoVGContext *ctxt;
        int            id;
    friend class NanoVGContext;
    friend class NanoVGTextCache;
};
class NanoVGTextCache final : public PaintResource {
    public:
        BTK_MAKE_PAINT_RESOURCE

        NanoVGTextCache(NanoVGContext *ctxt, const TextLayout &layout);
        ~NanoVGTextCache() = default;

        // Inhertied from PaintResource
        auto signal_destroyed() -> Signal<void()> & override {
            return ctxt->signal_text_cache_destroyed();
        }

        void draw(NVGcontext *ctxt, float x, float y);
    private:
        NanoVGContext *ctxt;
        Ref<NanoVGTexture> bitmap; //< Bitmap hold the data
        std::vector<Rect>  bounds;
};

class GLGuard {
    public:
        GLGuard(GLContext *ctxt) : ctxt(ctxt) {
            ctxt->begin();
        }
        GLGuard() {
            ctxt->end();
        }
    private:
        GLContext *ctxt;
};

// Helper for NVG
static void nvgRenderText(NVGcontext* ctx, int id, NVGvertex* verts, int nverts)
{
	NVGstate* state = nvg__getState(ctx);
	NVGpaint paint = state->fill;

	// Render triangles.
	paint.image = id;

	// Apply global alpha
	paint.innerColor.a *= state->alpha;
	paint.outerColor.a *= state->alpha;

	ctx->params.renderTriangles(ctx->params.userPtr, &paint, state->compositeOperation, &state->scissor, verts, nverts, ctx->fringeWidth);

	ctx->drawCallCount++;
	ctx->textTriCount += nverts/3;
}



NanoVGContext::NanoVGContext(NanoVGWindowDevice *dev) : device(dev), glctxt(dev->glctxt) {
    GLGuard guard(glctxt);

    OpenGLES3Function::load([&](const char *name) {
        return glctxt->get_proc_address(name);
    });

    // Query env
    GLint size[2];
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, size);
    max_texture_size.w = size[0];
    max_texture_size.h = size[1];

    int flags = NVG_STENCIL_STROKES | NVG_ANTIALIAS;

#if !defined(NDEBUG)
    flags |= NVG_DEBUG;

    // Debug
    BTK_LOG("[NanoVG::OpenGL] Version %s\n", glGetString(GL_VERSION));
    BTK_LOG("[NanoVG::OpenGL] Renderer %s\n", glGetString(GL_RENDERER));
    BTK_LOG("[NanoVG::OpenGL] Vendor %s\n", glGetString(GL_VENDOR));
    BTK_LOG("[NanoVG::OpenGL] Max Texture Size %d x %d\n", max_texture_size.w, max_texture_size.h);
#endif

    nvgctxt = nvgCreateGLES3(flags, this);
}
NanoVGContext::~NanoVGContext() {
    // Release 
    GLGuard guard(glctxt);

    signal_txt_cache.emit();
    signal.emit();

    nvgDeleteGLES3(nvgctxt);
}
void NanoVGContext::begin() {
    glctxt->begin();

    auto [glw, glh] = glctxt->get_drawable_size();
    auto [w, h] = device->size();

    glViewport(0, 0, glw, glh);

    nvgBeginFrame(nvgctxt, w, h, float(glw) / w);
}
void NanoVGContext::end() {
    nvgEndFrame(nvgctxt);

    if (numof_cache > max_cache_size) {
        BTK_LOG("[NanoVG::Text] Too much cache, cleaning\n");
        numof_cache = 0;

        // Destroy this cache
        signal_txt_cache.emit();
    }

    glctxt->end();
}
void NanoVGContext::swap_buffers() {
    glctxt->swap_buffers();
}

void NanoVGContext::clear(Brush &what) {
    BTK_ASSERT(what.type() == BrushType::Solid);
    auto [r, g, b, a] = what.color();

    glClearColor(r, g, b ,a);
    // glClearStencil(0);
    // glClearDepthf(0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);;
}

// Draw
bool NanoVGContext::draw_path(const PainterPath &path) {
    // TODO : Need calc position
    nvgBeginPath(nvgctxt);
    path.stream(this);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_line(float x1, float y1, float x2, float y2) {
    // Get Rectangle of the line
    if (need_apply_brush) {
        FRect rect;
        rect.x = min(x1, x2);
        rect.y = min(y1, y2);
        rect.w = max(x1, x2);
        rect.h = max(y1, y2);

        rect.h = rect.h - rect.y;
        rect.w = rect.w - rect.x;

        apply_brush(rect);
    }

    nvgBeginPath(nvgctxt);
    nvgMoveTo(nvgctxt, x1, y1);
    nvgLineTo(nvgctxt, x2, y2);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_rect(float x, float y, float w, float h) {
    if (w <= 0 || h <= 0) {
        return true;
    }

    apply_brush(x, y, w, h);

    nvgBeginPath(nvgctxt);
    nvgRect(nvgctxt, x, y, w, h);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_rounded_rect(float x, float y, float w, float h, float r) {
    apply_brush(x, y, w, h);

    nvgBeginPath(nvgctxt);
    nvgRoundedRect(nvgctxt, x, y, w, h, r);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_ellipse(float x, float y, float xr, float yr) {
    apply_brush(x - xr, y - yr, xr * 2, yr * 2);

    nvgBeginPath(nvgctxt);
    nvgEllipse(nvgctxt, x, y, xr, yr);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_image(AbstractTexture *image, const FRect *_dst, const FRect *_src) {
    auto nvgimage = static_cast<NanoVGTexture*>(image);
    auto tex_size = nvgimage->size();
    float tex_w = tex_size.w;
    float tex_h = tex_size.h;

    FRect dst;
    FRect src;

    if (_dst) {
        dst = *_dst;
    }
    else {
        auto [w, h] = device->size();
        dst = FRect(0, 0, w, h);
    }

    if (_src) {
        src = *_src;
    }
    else {
        src = FRect(0, 0, tex_w, tex_h);
    }

    if (dst.empty() || src.empty()) {
        return true;
    }

    // Zoom the image let it display at the right size
    
    float w_ratio = float(tex_w) / float(src.w);
    float h_ratio = float(tex_h) / float(src.h);

    // Transform the image to fit it
    FRect tgt;

    tgt.w = dst.w * w_ratio;
    tgt.h = dst.h * h_ratio;

    tgt.x = dst.x - (src.x / src.w) * dst.w;
    tgt.y = dst.y - (src.y / src.h) * dst.h;

    nvgSave(nvgctxt);
    nvgBeginPath(nvgctxt);
    nvgRect(nvgctxt, dst.x, dst.y, dst.w, dst.h);
    nvgFillPaint(nvgctxt, nvgImagePattern(
        nvgctxt,
        tgt.x,
        tgt.y,
        tgt.w,
        tgt.h,
        0,
        nvgimage->id,
        1.0f
    ));
    nvgFill(nvgctxt);
    nvgRestore(nvgctxt);
    return true;
}

// Text
bool NanoVGContext::draw_text(Alignment align, Font &font, u8string_view text, float x, float y) {
    TextLayout layout;
    layout.set_font(font);
    layout.set_text(text);

    auto [xdpi, ydpi] = device->dpi();
    auto [width, height] = layout.size();

    auto path = layout.outline(xdpi);
    
    if ((align & Alignment::Right) == Alignment::Right) {
        x -= width;
    }
    if ((align & Alignment::Bottom) == Alignment::Bottom) {
        y -= height;
    }
    if ((align & Alignment::Center) == Alignment::Center) {
        x -= width / 2;
    }
    if ((align & Alignment::Middle) == Alignment::Middle) {
        y -= height / 2;
    }

    apply_brush(x, y, width, height);

    nvgSave(nvgctxt);
    nvgShapeAntiAlias(nvgctxt, false);
    nvgTranslate(nvgctxt, x, y);
    fill_path(path);
    nvgRestore(nvgctxt);

    return true;
}
bool NanoVGContext::draw_text(Alignment align, const TextLayout &layout      , float x, float y) {
    auto [xdpi, ydpi] = device->dpi();
    auto [width, height] = layout.size();

    auto path = layout.outline(xdpi);
    
    if ((align & Alignment::Right) == Alignment::Right) {
        x -= width;
    }
    if ((align & Alignment::Bottom) == Alignment::Bottom) {
        y -= height;
    }
    if ((align & Alignment::Center) == Alignment::Center) {
        x -= width / 2;
    }
    if ((align & Alignment::Middle) == Alignment::Middle) {
        y -= height / 2;
    }

    apply_brush(x, y, width, height);

    nvgBeginPath(nvgctxt);
    auto resource = layout.query_device_resource(nvgctxt);
    if (!resource) {
        resource = new NanoVGTextCache(this, layout);
        layout.bind_device_resource(nvgctxt, resource);

        numof_cache += 1;
    }
    auto cache = static_cast<NanoVGTextCache*>(resource);
    cache->draw(nvgctxt, x, y);

    nvgFill(nvgctxt);
    return true;
}

// Fill
bool NanoVGContext::fill_path(const PainterPath &path) {
    nvgBeginPath(nvgctxt);
    path.stream(this);
    nvgFill(nvgctxt);

    return false;
}
bool NanoVGContext::fill_rect(float x, float y, float w, float h) {
    if (w <= 0 || h <= 0) {
        return true;
    }

    apply_brush(x, y, w, h);

    nvgBeginPath(nvgctxt);
    nvgRect(nvgctxt, x, y, w, h);
    nvgFill(nvgctxt);

    return false;
}
bool NanoVGContext::fill_rounded_rect(float x, float y, float w, float h, float r) {
    apply_brush(x, y, w, h);

    nvgBeginPath(nvgctxt);
    nvgRoundedRect(nvgctxt, x, y, w, h, r);
    nvgFill(nvgctxt);

    return false;
}
bool NanoVGContext::fill_ellipse(float x, float y, float xr, float yr) {
    apply_brush(x - xr, y - yr, xr * 2, yr * 2);

    nvgBeginPath(nvgctxt);
    nvgEllipse(nvgctxt, x, y, xr, yr);
    nvgFill(nvgctxt);

    return false;
}
bool NanoVGContext::fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) {
    // Unsupported
    return false;
}

// Vector Graphics Sink
void NanoVGContext::open() {
    winding = PathWinding::Hole;
}
void NanoVGContext::close() {

}
void NanoVGContext::move_to(float x, float y) {
    nvgMoveTo(nvgctxt, x, y);
}
void NanoVGContext::line_to(float x, float y) {
    nvgLineTo(nvgctxt, x, y);
}
void NanoVGContext::quad_to(float x1, float y1, float x2, float y2) {
    nvgQuadTo(nvgctxt, x1, y1, x2, y2);
}
void NanoVGContext::bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) { 
    nvgBezierTo(nvgctxt, x1, y1, x2, y2, x3, y3);
}
void NanoVGContext::arc_to(float x1, float y1, float x2, float y2, float radius) {
    nvgArcTo(nvgctxt, x1, y1, x2, y2, radius);
}
void NanoVGContext::close_path() {
    nvgClosePath(nvgctxt);

    // FIXME : Why Dwrite generated path make it hole to solod
    switch (winding) {
        case PathWinding::Hole : nvgPathWinding(nvgctxt, NVG_HOLE); break;
        case PathWinding::Solid : nvgPathWinding(nvgctxt, NVG_SOLID); break;
    }
}
void NanoVGContext::set_winding(PathWinding wi) {
    // TODO :
    winding = wi;
}

auto NanoVGContext::create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> AbstractTexture * {
    GLGuard guard(glctxt);

    int id = -1;
    if (fmt == PixFormat::RGBA32) {
        id = nvgCreateImageRGBA(nvgctxt, w, h, NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY, nullptr);
    }
    else if (fmt == PixFormat::Gray8) {
        // Alpha texture for render texture
        id = nvgctxt->params.renderCreateTexture(nvgctxt->params.userPtr, NVG_TEXTURE_ALPHA, w, h, 0, nullptr);
    }

    if (id < 0) {
        return nullptr;
    }
    return new NanoVGTexture(this, id);
}
bool NanoVGContext::set_state(PaintContextState state, const void *in) {
    switch (state) {
        case PaintContextState::Alpha : {
            nvgGlobalAlpha(nvgctxt, *static_cast<const float *>(in));
            break;
        }
        case PaintContextState::Antialias : {
            nvgShapeAntiAlias(nvgctxt, *static_cast<const bool *>(in));
            break;
        }
        case PaintContextState::StrokeWidth : {
            nvgStrokeWidth(nvgctxt, *static_cast<const float*>(in));
            break;
        }
        case PaintContextState::Transform : {
            nvgResetTransform(nvgctxt);
            if (in) {
                *current_transform() = *static_cast<const FMatrix*>(in);
            }
            break;
        }
        case PaintContextState::Scissor : {
            if (!in) {
                // No Scissor
                nvgResetScissor(nvgctxt);
            }
            else {
                auto *scissor = static_cast<const PaintScissor*>(in);
                auto &matrix = scissor->matrix;
                auto [x, y, w, h] = scissor->rect;

                // Content of nanovg scissor
                NVGstate* state = nvg__getState(nvgctxt);

                w = nvg__maxf(0.0f, w);
                h = nvg__maxf(0.0f, h);

                nvgTransformIdentity(state->scissor.xform);
                state->scissor.xform[4] = x+w*0.5f;
                state->scissor.xform[5] = y+h*0.5f;
                nvgTransformMultiply(state->scissor.xform, reinterpret_cast<const float*>(&matrix));

                state->scissor.extent[0] = w*0.5f;
                state->scissor.extent[1] = h*0.5f;
            }
            break;
        }
        case PaintContextState::Pen : {
            auto *pen = static_cast<const Pen*>(in);
            // Unsupport dash
            auto join = pen->line_join();
            auto cap = pen->line_cap();

            int nvg_join;
            int nvg_cap;

            switch (join) {
                case LineJoin::Miter: nvg_join = NVG_MITER; break;
                case LineJoin::Round: nvg_join = NVG_ROUND; break;
                case LineJoin::Bevel: nvg_join = NVG_BEVEL; break;
            }
            switch (cap) {
                case LineCap::Flat: nvg_cap = NVG_BUTT; break;
                case LineCap::Square: nvg_cap = NVG_SQUARE; break;
                case LineCap::Round: nvg_cap = NVG_ROUND; break;
            }

            nvgLineCap(nvgctxt, nvg_cap);
            nvgLineJoin(nvgctxt, nvg_join);
            nvgMiterLimit(nvgctxt, pen->miter_limit());
            break;
        }
        case PaintContextState::Brush : {
            auto *ibrush = static_cast<const Brush *>(in);

            if (ibrush->type() == BrushType::Solid) {
                auto [r, g, b, a] = ibrush->color();
                nvgStrokeColor(nvgctxt, nvgRGBAf(r, g, b, a));
                nvgFillColor(nvgctxt, nvgRGBAf(r, g, b, a));
                need_apply_brush = false;
            }
            else {
                // More complex brush
                brush = *ibrush;
                need_apply_brush = true;
            }
            break;
        }
        default : return false;
    }
    return true;
}
bool NanoVGContext::native_handle(PaintContextHandle what, void *out) {
    if (what == PaintContextHandle::GLContext) {
        *static_cast<GLContext**>(out) = glctxt;
        return true;
    }
    return false;
}
void NanoVGContext::apply_brush(const FRect &object) {
    if (!need_apply_brush) {
        return;
    }
    auto type = brush.type();
    // It could not be normal color
    BTK_ASSERT(type != BrushType::Solid);

    // TODO : NanoVG Unsupport offset, we need to enuallfy the behavior


    switch (type) {
        case BrushType::LinearGradient : {
            auto &lg = brush.linear_gradient();
            auto &stops = lg.stops();

            BTK_ASSERT(stops.size() >= 2);
            auto first = stops[0].color;
            auto second = stops[1].color;
            
            auto start_point = brush.point_to_abs(device, object, lg.start_point());
            auto end_point = brush.point_to_abs(device, object, lg.end_point());

            auto paint = nvgLinearGradient(
                nvgctxt, 
                start_point.x, start_point.y, 
                end_point.x, end_point.y, 
                nvgRGBAf(first.r, first.g, first.b, first.a), 
                nvgRGBAf(second.r, second.g, second.b, second.a)
            );

            nvgFillPaint(nvgctxt, paint);
            nvgStrokePaint(nvgctxt, paint);
            break;
        }
        case BrushType::RadialGradient : {
            // FIXME : Bug here
            auto &rg = brush.radial_gradient();
            auto &stops = rg.stops();

            BTK_ASSERT(stops.size() >= 2);
            auto first = stops[0].color;
            auto second = stops[1].color;
            
            auto center_point = brush.point_to_abs(device, object, rg.center_point());
            auto radius = brush.point_to_abs(device, object, FPoint(rg.radius_x(), rg.radius_y()));;

            auto paint = nvgRadialGradient(
                nvgctxt, 
                center_point.x, center_point.y, 
                0, radius.y, 
                nvgRGBAf(first.r, first.g, first.b, first.a), 
                nvgRGBAf(second.r, second.g, second.b, second.a)
            );

            nvgFillPaint(nvgctxt, paint);
            nvgStrokePaint(nvgctxt, paint);
            break;
        }
        case BrushType::Bitmap : {
            // Bitmap brush
            auto pixbuffer = brush.bitmap();
            auto resource = brush.query_device_resource(nvgctxt);
            if (!resource) {
                // Create a bitmap
                auto tex = create_texture(pixbuffer.format(), pixbuffer.width(), pixbuffer.height(), 96, 96);
                tex->update(nullptr, pixbuffer.pixels(), pixbuffer.pitch());

                resource = tex;
                brush.bind_device_resource(nvgctxt, tex);
            }
            auto tex = static_cast<NanoVGTexture*>(resource);

            // Calc the position
            auto rect = brush.rect_to_abs(device, object, brush.rect());
            auto paint = nvgImagePattern(
                nvgctxt,
                rect.x, rect.y, rect.w, rect.h,
                0,
                tex->id,
                1.0f
            );
            nvgFillPaint(nvgctxt, paint);
            nvgStrokePaint(nvgctxt, paint);
            break;
        }
    }
}
void NanoVGContext::apply_brush(float x, float y, float w, float h) {
    apply_brush(FRect(x, y, w, h));
}
auto NanoVGContext::current_transform() -> FMatrix * {
    NVGstate* s = nvg__getState(nvgctxt);

    static_assert(sizeof(s->xform) == sizeof(FMatrix));
    return reinterpret_cast<FMatrix*>(s->xform);
}

NanoVGTexture::~NanoVGTexture() {
    auto glctxt = ctxt->gl_context();
    auto nvgctxt = ctxt->nvg_context();

    GLGuard guard(glctxt);
    nvgDeleteImage(nvgctxt, id);
}
void NanoVGTexture::update(const Rect *r, cpointer_t data, int pitch) {
    GLGuard guard(ctxt->gl_context());
    nvgUpdateImage(ctxt->nvg_context(), id, static_cast<const uint8_t*>(data));
}
void NanoVGTexture::set_interpolation_mode(InterpolationMode mode) {
    // TODO 
    GLGuard guard(ctxt->gl_context());

    auto tex = glnvg__findTexture((GLNVGcontext*) nvgInternalParams(ctxt->nvg_context()), id);
    if (tex == nullptr) {
        return;
    }

    GLint prev_tex;
    ctxt->glGetIntegerv(GL_TEXTURE_2D, &prev_tex);
    ctxt->glBindTexture(GL_TEXTURE_2D, tex->id);

	if (mode == InterpolationMode::Nearest) {
		ctxt->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} 
    else {
		ctxt->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

    ctxt->glBindTexture(GL_TEXTURE_2D, prev_tex);
}
bool NanoVGTexture::query_value(PaintDeviceValue value, void *out) {
    switch (value) {
        case PaintDeviceValue::PixelSize : {
            auto fs = static_cast<Size*>(out);
            nvgImageSize(ctxt->nvg_context(), id, &fs->w, &fs->h);
            break;
        }
        case PaintDeviceValue::LogicalSize : {
            auto fs = static_cast<Size*>(out);
            nvgImageSize(ctxt->nvg_context(), id, &fs->w, &fs->h);
            break;
        }
        case PaintDeviceValue::Dpi : {
            auto fp = static_cast<FPoint*>(out);
            fp->x = 96.0f;
            fp->y = 96.0f;
            break;
        }
        default : {
            return false;
        }
    }
    return true;
}

// NanoVG Text cache
NanoVGTextCache::NanoVGTextCache(NanoVGContext *ctxt, const TextLayout &layout) : ctxt(ctxt) {
    std::vector<PixBuffer> bitmaps;

    auto [xdpi, ydpi] = ctxt->device->dpi();
    layout.rasterize(xdpi, &bitmaps, &bounds);

    int width = 0, height = 0;
    for (auto &rect : bounds) {
        // BTK_ASSERT(rect.x >= 0);
        // BTK_ASSERT(rect.h >= 0);

        width = max(width, rect.x + rect.w);
        height = max(height, rect.y + rect.h);
    }

    // Make Bitmap on it
    bitmap = static_cast<NanoVGTexture *>(ctxt->create_texture(PixFormat::Gray8, width, height, 96, 96));

    // Because nanovg use a big texture to update a part of it, So we also like it
    PixBuffer hole_texture(PixFormat::Gray8, width, height);
    auto pixels = hole_texture.pixels<uint8_t>();

    Btk_memset(pixels, 0, width * height);

    int i = 0;
    for (auto &buf : bitmaps) {
        // Copy part of it 
        auto &rect = bounds[i];
        auto src = buf.pixels<uint8_t>();
        auto dst = pixels;

        for (int y = 0; y < rect.h; y++) {
            for (int x = 0; x < rect.w; x++) {
                dst[(y + rect.y) * width + (x + rect.x)] = src[y * buf.pitch() + x];
            }
        }

        i += 1;
    }

    bitmap->update(nullptr, hole_texture.pixels(), hole_texture.pitch());
}

void NanoVGTextCache::draw(NVGcontext *nvgctxt, float x, float y) {
    // TODO : Better cache way
    class Quad {
        public:
            float x0,y0,s0,t0;
            float x1,y1,s1,t1;
            // X Y Is 
    };

    int cverts = bounds.size() * 6;
    int nverts = 0;
    NVGvertex *verts = nvg__allocTempVerts(nvgctxt, cverts);
    NVGstate *state = nvg__getState(nvgctxt);

    float invscale = 1.0f / nvgctxt->devicePxRatio;

    auto [tex_w, tex_h] = bitmap->pixel_size();
    float itw = 1.0f / tex_w;
    float ith = 1.0f / tex_h;

	int isFlipped = nvg__isTransformFlipped(state->xform);

    // X & Y Is In DiP, so convert it back
    x /= invscale;
    y /= invscale;


    // Render each glyph
    for (auto &rect : bounds) {
        float c[4*2];

        // Make Quad
        Quad q;
        q.x0 = x + rect.x;
        q.x1 = x + rect.x + rect.w;
        q.y0 = y + rect.y;
        q.y1 = y + rect.y + rect.h;

        // Calc in texture
        q.s0 = rect.x * itw;
        q.s1 = (rect.x + rect.w) * itw;
        q.t0 = rect.y * ith;
        q.t1 = (rect.y + rect.h) * ith;

		if(isFlipped) {
			float tmp;

			tmp = q.y0; q.y0 = q.y1; q.y1 = tmp;
			tmp = q.t0; q.t0 = q.t1; q.t1 = tmp;
		}

		// Transform corners.
		nvgTransformPoint(&c[0],&c[1], state->xform, q.x0*invscale, q.y0*invscale);
		nvgTransformPoint(&c[2],&c[3], state->xform, q.x1*invscale, q.y0*invscale);
		nvgTransformPoint(&c[4],&c[5], state->xform, q.x1*invscale, q.y1*invscale);
		nvgTransformPoint(&c[6],&c[7], state->xform, q.x0*invscale, q.y1*invscale);
		// Create triangles
		if (nverts+6 <= cverts) {
			nvg__vset(&verts[nverts], c[0], c[1], q.s0, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[4], c[5], q.s1, q.t1); nverts++;
			nvg__vset(&verts[nverts], c[2], c[3], q.s1, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[0], c[1], q.s0, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[6], c[7], q.s0, q.t1); nverts++;
			nvg__vset(&verts[nverts], c[4], c[5], q.s1, q.t1); nverts++;
		}

    }
    nvgRenderText(nvgctxt, bitmap->id, verts, nverts);
}

// NanoVG Device
NanoVGWindowDevice::NanoVGWindowDevice(AbstractWindow *wi) : window(wi) {
    glctxt = static_cast<GLContext*>(window->gc_create("opengl"));
    if (!glctxt) {
        BTK_THROW(std::runtime_error("Could not create OpenGL context"));
    }
    // Try init
    GLFormat fmt = {};
    if (!glctxt->initialize(fmt)) {
        delete glctxt;
        glctxt = nullptr;
        BTK_THROW(std::runtime_error("Could not init OpenGL context"));
    }

    nvgctxt = new NanoVGContext(this);
}
NanoVGWindowDevice::~NanoVGWindowDevice() {
    delete nvgctxt;
    delete glctxt;
}

bool NanoVGWindowDevice::query_value(PaintDeviceValue value, void *out) {
    switch (value) {
        case PaintDeviceValue::PixelSize : {
            auto fs = static_cast<Size*>(out);
            GLGuard guard(glctxt);
            *fs = glctxt->get_drawable_size();
            break;
        }
        case PaintDeviceValue::LogicalSize : {
            auto fs = static_cast<FSize*>(out);
            *fs = window->size();
            break;
        }
        case PaintDeviceValue::Dpi : {
            auto fp = static_cast<FPoint*>(out);
            return window->query_value(AbstractWindow::Dpi, fp);            
        }
    }
    return true;
}
auto NanoVGWindowDevice::paint_context() -> PaintContext* {
    return nvgctxt;
}

extern "C" {
    void __BtkPlatform_NVG_Init() {
        RegisterPaintDevice<AbstractWindow>([](AbstractWindow *win) -> PaintDevice * {
            BTK_TRY {
                return new NanoVGWindowDevice(win);
            }
            BTK_CATCH (std::exception &err) {
                printf("WARN %s\n", err.what());
                return nullptr;
            }
        });
    }
}

BTK_PRIV_END