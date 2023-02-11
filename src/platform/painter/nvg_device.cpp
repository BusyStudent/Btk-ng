#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/opengl/opengl.hpp>
#include <Btk/painter.hpp>

extern "C" {
    #define NANOVG_GLES3_IMPLEMENTATION
    #include "libs/nanovg.hpp"
    #include "libs/nanovg_gl.hpp"
}

BTK_PRIV_BEGIN

class NanoVGWindowDevice final : public WindowDevice {
    public:
        NanoVGWindowDevice();

    private:
        AbstractWindow *window;
        GLContext      *glctxt;
};

class NanoVGContext final : public PaintContext, PainterPathSink, OpenGLES3Function {
    public:
        NanoVGContext();
        ~NanoVGContext();

        GLContext *gl_context() const;
        NVGcontext *nvg_context() const;

        void begin() override;
        void end() override;
        void swap_buffers() override;

        // Inhert from PaintContext
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
    private:
        NanoVGWindowDevice *device;
        GLContext *glctxt;
        NVGcontext *nvgctxt;

        Signal<void()> signal; //< Signal for cleanup resource
};
class NanoVGTexture final : public AbstractTexture {
    public:
        NanoVGTexture(NanoVGContext *ctxt, int id);
        ~NanoVGTexture();

        // Inhertied from PaintResource

        auto signal_destroyed() -> Signal<void()> & override {
            return ctxt->signal_destroyed();
        }

        void update(const Rect *area, cpointer_t ptr, int pitch) override;
    private:
        NanoVGContext *ctxt;
        int            id;
};


NanoVGContext::NanoVGContext() {
    OpenGLES3Function::load(std::bind(&GLContext::get_proc_address, glctxt));

    glctxt->begin();
    nvgctxt = nvgCreateGLES3(NVG_STENCIL_STROKES | NVG_ANTIALIAS | NVG_DEBUG, this);
    glctxt->end();
}
NanoVGContext::~NanoVGContext() {
    // Release 
    glctxt->end();

    signal.emit();

    nvgDeleteGLES3(nvgctxt);
    glctxt->end();
}
void NanoVGContext::begin() {
    glctxt->begin();

    auto [glw, glh] = glctxt->get_drawable_size();
    auto [w, h] = device->size();

    nvgBeginFrame(nvgctxt, w, h, float(glw) / w);
}
void NanoVGContext::end() {
    nvgEndFrame(nvgctxt);
}
void NanoVGContext::swap_buffers() {
    glctxt->swap_buffers();
}

// Draw
bool NanoVGContext::draw_path(const PainterPath &path) {
    nvgBeginPath(nvgctxt);
    path.stream(this);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_line(float x1, float y1, float x2, float y2) {
    nvgBeginPath(nvgctxt);
    nvgMoveTo(nvgctxt, x1, y1);
    nvgLineTo(nvgctxt, x2, y2);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_rect(float x, float y, float w, float h) {
    nvgBeginPath(nvgctxt);
    nvgRect(nvgctxt, x, y, w, h);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_rounded_rect(float x, float y, float w, float h, float r) {
    nvgBeginPath(nvgctxt);
    nvgRoundedRect(nvgctxt, x, y, w, h, r);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_ellipse(float x, float y, float xr, float yr) {
    nvgBeginPath(nvgctxt);
    nvgEllipse(nvgctxt, x, y, xr, yr);
    nvgStroke(nvgctxt);

    return true;
}
bool NanoVGContext::draw_image(AbstractTexture *image, const FRect *dst, const FRect *src) {
    auto nvgimage = static_cast<NanoVGTexture*>(image);
    nvgSave(nvgctxt);
}

// Text
bool NanoVGContext::draw_text(Alignment align, Font &font, u8string_view text, float x, float y) {
    TextLayout layout;
    layout.set_font(font);
    layout.set_text(text);

    return draw_text(align, layout, x, y);
}
bool NanoVGContext::draw_text(Alignment align, const TextLayout &layout      , float x, float y) {
    auto path = layout.outline();
    return fill_path(path);
}

// Fill
bool NanoVGContext::fill_path(const PainterPath &path) {

}
bool NanoVGContext::fill_rect(float x, float y, float w, float h) {

}
bool NanoVGContext::fill_rounded_rect(float x, float y, float w, float h, float r) {

}
bool NanoVGContext::fill_ellipse(float x, float y, float xr, float yr) {

}
bool NanoVGContext::fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) {

}

NanoVGTexture::~NanoVGTexture() {
    auto glctxt = ctxt->gl_context();
    auto nvgctxt = ctxt->nvg_context();

    glctxt->begin();
    nvgDeleteImage(nvgctxt, id);
    glctxt->end();
}

BTK_PRIV_END