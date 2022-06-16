#include "build.hpp"

#include <Btk/context.hpp>
#include <cairo/cairo.h>

#if   defined(__gnu_linux__) && !defined(BTK_CAIRO_XCB)
#include <cairo/cairo-xlib.h>
#elif defined(__gnu_linux__) && defined(BTK_CAIRO_XCB)
#include <cairo/cairo-xcb.h>
#else
#error "Unsupported platform"
#endif

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

class CairoGraphicsContext : public GraphicsContext {
    public:
        CairoGraphicsContext(cairo_surface_t *surf);
        ~CairoGraphicsContext();

        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

        static gcontext_t From(pointer_t native_window);
    private:
        cairo_surface_t *surface; //< Target surface
        cairo_t *cr; //< Cairo context
};

CairoGraphicsContext::CairoGraphicsContext(cairo_surface_t *surf) {
    surface = surf;
    cr = cairo_create(surface);
}
CairoGraphicsContext::~CairoGraphicsContext() {
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
}
// void CairoGraphicsContext::set_color(float r, float g, float b, float a) {
//     cairo_set_source_rgba(cr, r, g, b, a);
// }

BTK_NS_END2()