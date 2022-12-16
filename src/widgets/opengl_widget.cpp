#include "build.hpp"

#include <Btk/opengl/opengl_widget.hpp>
#include <Btk/opengl/opengl.hpp>
#include <Btk/context.hpp>

BTK_NS_BEGIN

// TODO : Need a better way to draw OpenGL framebuffer by painter

GLWidget::GLWidget() {

}
GLWidget::~GLWidget() {
    delete glctxt;
    delete glwin;

    Btk_free(buffer);
}

bool GLWidget::paint_event(PaintEvent &event) {
    if (!glwin) {
        gl_initialize();
    }
    if (!glctxt) {
        // Error
        painter().set_color(Color::Black);
        painter().fill_rect(rect());
        return true;
    }
    auto ctxt = static_cast<GLContext*>(glctxt);
    auto glReadPixels = PFNGLREADPIXELSPROC(ReadPixels);
    auto glGetError   = PFNGLGETERRORPROC(GetError);
    auto glViewport  = PFNGLVIEWPORTPROC(Viewport);


    ctxt->begin();
    // Query info
    Size glsize = ctxt->get_drawable_size();
    if (buffer_size != glsize || !buffer) {
        buffer_size = glsize;
        buffer = Btk_realloc(buffer, 4 * buffer_size.w * buffer_size.h);
    }

    glViewport(0, 0, glsize.w, glsize.h);
    BTK_ASSERT(glGetError() == 0);

    // Begin Draw
    gl_paint();

    // Read the pixel back
    glReadPixels(
        0, 0, glsize.w, glsize.h,
        GL_RGBA, GL_UNSIGNED_BYTE, buffer
    );
    BTK_ASSERT(glGetError() == 0);

    ctxt->end();

    // Make texture
    auto &p = painter();
    if (!texture.empty()) {
        if (texture.size() != glsize) {
            texture.clear();
        }
    }
    if (texture.empty()) {
        texture = p.create_texture(PixFormat::RGBA32, glsize.w, glsize.h);
    }
    texture.update(nullptr, buffer, glsize.w * 4);

    // Copy to screen
    FRect dst = rect();
    p.draw_image(texture, &dst, nullptr);

    return true;
}
bool GLWidget::resize_event(ResizeEvent &event) { 
    if (glwin) {
        glwin->resize(event.width(), event.height());
    }
    return true;
}
void  GLWidget::gl_initialize() {
    auto size = rect().size();
    glwin = driver()->window_create(
        nullptr,
        size.w,
        size.h,
        WindowFlags::OpenGL | WindowFlags::NeverShowed
    );
    if (!glwin) {
        return;
    }
    glctxt = glwin->gc_create("opengl");
    if (!glctxt) {
        return;
    }

    auto ctxt = static_cast<GLContext*>(glctxt);

#if !defined(BTK_NO_RTTI)
    // Check is OpenGL context
    if (!dynamic_cast<GLContext*>(glctxt)) {
        goto error;
    }
#endif


    if (!ctxt->initialize(GLFormat())) {
        goto error;
    }

    // Get some function pointers
    ReadPixels = ctxt->get_proc_address("glReadPixels");
    Viewport   = ctxt->get_proc_address("glViewport");
    GetError = ctxt->get_proc_address("glGetError");


    // Notify User
    ctxt->begin();
    gl_ready();
    ctxt->end();

    return;

    error:
        delete glctxt;
        delete glwin;

        glctxt = nullptr;
        glwin = nullptr;
        return;
}

void *GLWidget::gl_get_proc_address(const char_t *proc_name) {
    if (!glwin) {
        return nullptr;
    }
    return static_cast<GLContext*>(glctxt)->get_proc_address(proc_name);
}
Size  GLWidget::gl_drawable_size() {
    if (!glwin) {
        return Size(0, 0);
    }
    return static_cast<GLContext*>(glctxt)->get_drawable_size();
}
bool   GLWidget::gl_make_current() {
    if (!glwin) {
        return false;
    }
    return static_cast<GLContext*>(glctxt)->make_current();
}

BTK_NS_END