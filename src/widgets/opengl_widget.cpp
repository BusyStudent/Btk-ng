#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/opengl/opengl_widget.hpp>
#include <Btk/opengl/opengl.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

// TODO : Need a better way to draw OpenGL framebuffer by painter

// Delegate
class GLWidgetImpl : public OpenGLES3Function {
    public:
        std::unique_ptr<GLContext> glctxt;
        bool ok = false;

        virtual ~GLWidgetImpl() { }
        virtual bool handle(Event &) = 0;
        virtual void gl_paint(GLWidget *) = 0;
};
// Slowest way, but portable
class GLWidgetOffscreenFbImpl : public GLWidgetImpl {
    public:
        GLWidgetOffscreenFbImpl(GLWidget *w);
        ~GLWidgetOffscreenFbImpl();
        bool handle(Event &) override;
        void gl_paint(GLWidget *) override;

        std::unique_ptr<AbstractWindow> glwin;
        void *membuffer = nullptr;
        size_t membuffer_size = 0;
        Texture texture;
};
class GLWidgetShareFbImpl : public GLWidgetImpl {

};
class GLWidgetWin32GLImpl : public GLWidgetImpl {
    public:

};

GLWidget::GLWidget() {
    priv = nullptr;
}
GLWidget::~GLWidget() {
    delete priv;
}

bool GLWidget::paint_event(PaintEvent &event) {
    if (!priv && !init_failed) {
        if (!gl_initialize()) {
            init_failed = true;
        }
    }
    if (init_failed) {
        painter().set_color(Color::Black);
        painter().fill_rect(rect());
        return true;
    }
    priv->gl_paint(this);
    return true;
}
bool GLWidget::resize_event(ResizeEvent &event) { 
    if (priv) {
        return priv->handle(event);
    }
    return true;
}
bool GLWidget::move_event(MoveEvent &event) {
    if (priv) {
        return priv->handle(event);
    }
    return true;
}
bool  GLWidget::gl_initialize() {
    priv = new GLWidgetOffscreenFbImpl(this);
    if (!priv->ok) {
        delete priv;
        return false;
    }

    // Done
    priv->glctxt->begin();
    gl_ready();
    priv->glctxt->end();

    return true;
}

void *GLWidget::gl_get_proc_address(const char_t *proc_name) {
    if (priv) {
        return priv->glctxt->get_proc_address(proc_name);
    }
    return nullptr;
}
Size  GLWidget::gl_drawable_size() {
    if (priv) {
        return priv->glctxt->get_drawable_size();
    }
    return Size(-1, -1);
}
bool   GLWidget::gl_make_current() {
    if (priv) {
        return priv->glctxt->make_current();
    }
    return false;
}
GLContext *GLWidget::gl_context() {
    if (priv) {
        return priv->glctxt.get();
    }
    return nullptr;
}

// GLWidgetOffscreenFbImpl
GLWidgetOffscreenFbImpl::GLWidgetOffscreenFbImpl(GLWidget *widget) {
    auto driver = widget->driver();

    // Create window used for Framebuffer
    glwin.reset(
        driver->window_create(
            "GLWidgetFramebuffer",
            widget->width(),
            widget->height(),
            WindowFlags::OpenGL | WindowFlags::Framebuffer
        )
    );
    if (!glwin) {
        // Bad
        return;
    }
    // Try alloc glcontext
    glctxt.reset(static_cast<GLContext*>(glwin->gc_create("opengl")));
    if (!glctxt) {
        return;
    }

    // Try init
    if (!glctxt->initialize(GLFormat())) {
        return;   
    }

    glctxt->begin();

    // Load proc
    OpenGLES3Function::load([this](const char *name) {
        return glctxt->get_proc_address(name);
    });

    // Alloc buffer
    auto [dw, dh] = glctxt->get_drawable_size();
    membuffer = Btk_realloc(membuffer, dw * dh * 4);
    membuffer_size = dw * dh * 4;

    glctxt->end();

    ok = true;
}
GLWidgetOffscreenFbImpl::~GLWidgetOffscreenFbImpl() {
    glctxt.reset();
    glwin.reset();
    Btk_free(membuffer);
}
bool GLWidgetOffscreenFbImpl::handle(Event &event) {
    switch (event.type()) {
        case Event::Resized : {
            auto w = event.as<ResizeEvent>().width();
            auto h = event.as<ResizeEvent>().height();
            glwin->resize(w, h);

            texture.clear();

            glctxt->begin();
            auto [dw, dh] = glctxt->get_drawable_size();
            if (membuffer_size < dw * dh * 4) {
                membuffer_size = dw * dh * 4;
                membuffer = Btk_realloc(membuffer, membuffer_size);
            }
            glctxt->end();
            return true;
        }
    }
    return false;
}
void GLWidgetOffscreenFbImpl::gl_paint(GLWidget *widget) {
    // Begin ctxt
    glctxt->begin();

    // Let child paint
    auto [dw, dh] = glctxt->get_drawable_size();
    glViewport(0, 0, dw, dh);
    widget->gl_paint();

    // Read pixels
    glViewport(0, 0, dw, dh);
    glReadPixels(0, 0, dw, dh, GL_RGBA, GL_UNSIGNED_BYTE, membuffer);
    if (glGetError() != GL_NO_ERROR) {
        printf("Ogl errpr\n");
    }

    auto &painter = widget->painter();
    FRect dst = widget->rect();
    // Make texture
    if (texture.empty()) {
        texture = painter.create_texture(PixFormat::RGBA32, dw, dh);
    }
    texture.update(nullptr, membuffer, dw * 4);
    painter.draw_image(texture, &dst, nullptr);

    glctxt->end();
}

BTK_NS_END