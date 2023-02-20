/*
#if 0
#include "build.hpp"
#include "fallback.hpp"
#include "common/utils.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <pango/pangocairo.h>
#include <cstdlib>
#include <cairo.h>
#include <dlfcn.h>
#include <map>
#include <stdexcept>

#if CAIRO_HAS_XCB_SURFACE
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <cairo-xcb.h>
#endif

#if CAIRO_HAS_XLIB_SURFACE
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <cairo-xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#define FONT_CAST(ptr) ((PangoFontDescription*&)ptr)

namespace {
    //< For Easy create Text layout
    using namespace BTK_NAMESPACE;

    static cairo_surface_t *mem_surf = nullptr;
    static cairo_t *mem_cr = nullptr;
    static int      mem_refcount = 0;

    void layout_translate_by_align(PangoLayout *layout, Alignment align, float w, float h, float &x, float &y) {
        // Begin translate to left,top
        if ((align & Alignment::Top) == Alignment::Top) {
            // Nothing
        }
        else if((align & Alignment::Middle) == Alignment::Middle) {
            y = y - h / 2.0;
        }
        else if((align & Alignment::Bottom) == Alignment::Bottom){
            y = y - h;
        }


        if ((align & Alignment::Left) == Alignment::Left) {
            // Nothing
        }
        else if((align & Alignment::Center) == Alignment::Center) {
            x = x - w / 2.0;
        }
        else if((align & Alignment::Right) == Alignment::Right){
            x = x - w;
        }
    }

    FRect brush_rect_to_abs(CoordinateMode m, const FRect &rel, const FRect object) {
        switch (m) {
            case CoordinateMode::Absolute : {
                return rel;
            }
            case CoordinateMode::Relative : {
                FRect ret;
                ret.x = object.x + rel.x * object.w;
                ret.y = object.y + rel.y * object.h;
                ret.w = object.w * rel.w;
                ret.h = object.h * rel.h;
                return ret;
            }
            default : {
                abort();
            }
        }
    }

    auto btk_fmt_to_cr(PixFormat fmt) -> cairo_format_t {
        switch (fmt) {
            case PixFormat::RGBA32 : {
                return CAIRO_FORMAT_ARGB32;
            }
            case PixFormat::RGB24 : {
                return CAIRO_FORMAT_RGB24;
            }
            case PixFormat::Gray8 : {
                return CAIRO_FORMAT_A8;
            }
            default : {
                abort();
            }
        }
    }

    uint32_t rgba32_to_argb(uint32_t pix) {
        auto dst = (uint8_t*)&pix;
        uint32_t r = dst[0];
        uint32_t g = dst[2];

        dst[0] = g;
        dst[2] = r;
        return pix;
    }
    uint32_t argb_to_rgb32(uint32_t pix) {
        auto dst = (uint8_t*)&pix;
        uint32_t g = dst[0];
        uint32_t r = dst[2];

        dst[0] = r;
        dst[2] = g;
        return pix;
    }
}


BTK_NS_BEGIN

class TextLayoutImpl : public Refable<TextLayoutImpl>, public Trackable {
    public:
        PangoLayout *lazy_eval(cairo_t *ctxt = nullptr);

        GPointer<PangoLayout> layout;
        Font                  font;
        cairo_t              *cr = nullptr; //< Which cario ctxt belong
        u8string              text;
        int64_t               text_len = -1;
};
class TextureImpl   : public Trackable {
    public:
        TextureImpl(PainterImpl *p, PixFormat fmt, int w, int h);
        TextureImpl(const TextureImpl &) = delete;
        ~TextureImpl();

        bool xlib_create(PainterImpl *p, PixFormat fmt, int w, int h);
        void xlib_update(const Rect *where, cpointer_t data, uint32_t pitch);
        void xlib_destroy();
        Size xlib_get_size();

        bool sw_create(PainterImpl *p, PixFormat fmt, int w, int h);
        void sw_update(const Rect *where, cpointer_t data, uint32_t pitch);
        void sw_destroy();
        Size sw_get_size();


        // Method
        decltype(&TextureImpl::sw_get_size) get_size = nullptr;
        decltype(&TextureImpl::sw_destroy)  destroy = nullptr;
        decltype(&TextureImpl::sw_update)   update = nullptr;

        // Internal data
        cairo_surface_t *surface = nullptr;
        cairo_pattern_t *pattern = nullptr;

#if     0
        // Xlib shm data
        XShmSegmentInfo shminfo = {};
        XImage         *image   = {};
        Window          window  = {};
        GC              gc      = {};
#endif
};
class BrushImpl : public Refable<BrushImpl> {
    public:
        BrushImpl() {

        }
        BrushImpl(const BrushImpl &b) {
            pat = cairo_pattern_reference(b.pat);
            // data = b.data;
            mode = b.mode;
            type = b.type;
            rect = b.rect;
        }
        ~BrushImpl() {
            cairo_pattern_destroy(pat);
        }

        cairo_pattern_t *pat = nullptr;
        FRect            rect = {0.0f, 0.0f, 1.0f, 1.0f};
        CoordinateMode   mode = CoordinateMode::Relative;
        BrushType        type = BrushType::Solid;


        // std::variant<LinearGradient, RadialGradient, PixBuffer> data;
};
class PenImpl : public Refable<PenImpl> {
    public:
        std::vector<double> dashes;
        double              dash_offset = 0;
        double              miter_limit = 10.0;
        cairo_line_join_t   join = CAIRO_LINE_JOIN_MITER;
        cairo_line_cap_t    cap  = CAIRO_LINE_CAP_BUTT;
};

class PainterPathImpl {
    public:
        PainterPathImpl() {
            auto surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
            cr = cairo_create(surf);
            cairo_surface_destroy(surf);
        }
        ~PainterPathImpl() {
            cairo_path_destroy(path);
            cairo_destroy(cr);
        }
        cairo_path_t *path = nullptr;
        cairo_t      *cr = nullptr;
        float         last_x = 0;
        float         last_y = 0;
};
class PainterImpl {
    public:
        PainterImpl(::Display *dpy, ::Window xwin);
        PainterImpl(PixBuffer &buffer);
        ~PainterImpl();

        void apply_brush(const FRect &box);
        void notify_resize(int w, int h);
        void initialize();

        cairo_surface_t *surf;
        cairo_t         *cr;

        // Text Layout pgo ctxt
        PangoLayout     *layout;
        Alignment        align = Alignment::Left | Alignment::Top;

        // Brush
        Ref<BrushImpl>   brush;

        // Backend callback
        Size (*get_surface_size)(PainterImpl *self) = nullptr;
        void (*set_surface_size)(PainterImpl *self, int w, int h) = nullptr;
        void (*flush_surface)(PainterImpl *self) = nullptr;

        Size (*get_framebuffer_size)(PainterImpl *self) = nullptr;

        // Default value
        cairo_line_join_t default_join;
        cairo_line_cap_t  default_cap;
        double            default_miter_limit;

        // From abstract window if
        AbstractWindow   *window = nullptr;

        struct {
            uint8_t xlib : 1;
            uint8_t xcb  : 1;
            uint8_t sw  : 1;
        } target_opt;

        union {
            PixBuffer *bitmap;

#if         CAIRO_HAS_XLIB_SURFACE
            struct {
               Display *display;
               Window   window; 
               bool     shm;
            } xlib;
#endif

#if         CAIRO_HAS_XCB_SURFACE
            struct {
                xcb_connection_t *connection;
                xcb_window_t      window;
            } xcb;
#endif

        } target_info;
};

inline PainterImpl::PainterImpl(::Display *dpy, ::Window xwin) {

#if CAIRO_HAS_XLIB_SURFACE
    XWindowAttributes attrs;
    XGetWindowAttributes(dpy, xwin, &attrs);

    // Make surface
    surf = cairo_xlib_surface_create(
        dpy,
        xwin,
        attrs.visual,
        attrs.width,
        attrs.height
    );
    if (!surf) {
        // Handle err
    }
    cr = cairo_create(surf);
    if (!cr) {

    }

    // Set target info
    Btk_memset(&target_opt, 0, sizeof(target_opt));
    target_opt.xlib = true;
    
    Btk_memset(&target_info, 0, sizeof(target_info));
    target_info.xlib.display = dpy;
    target_info.xlib.window = xwin;

#if 0
    target_info.xlib.shm = XShmQueryExtension(dpy);
#endif

    // Set backend cb
    get_surface_size = [](PainterImpl *self) {
        Size s;
        s.w = cairo_xlib_surface_get_width(self->surf);
        s.h = cairo_xlib_surface_get_height(self->surf);
        return s;
    };
    set_surface_size = [](PainterImpl *self, int w, int h) {
        cairo_xlib_surface_set_size(self->surf, w, h);
    };
    get_framebuffer_size = [](PainterImpl *self) {
        XWindowAttributes attrs;
        Size s;
        XGetWindowAttributes(
            self->target_info.xlib.display,
            self->target_info.xlib.window,
            &attrs
        );
        s.w = attrs.width;
        s.h = attrs.height;
        return s;
    };

    // Done
    initialize();
#else
    BTK_THROW(std::runtime_error("!CAIRO_HAS_XLIB_SURFACE"));
#endif

}
inline PainterImpl::PainterImpl(PixBuffer &buf) {
    surf = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        buf.width(),
        buf.height()
    );
    if (!surf) {

    }
    cr = cairo_create(surf);
    if (!cr) {

    }

    // Set target opt
    Btk_memset(&target_opt, 0, sizeof(target_opt));
    target_opt.sw = true;

    // Set target info
    Btk_memset(&target_info, 0, sizeof(target_info));
    target_info.bitmap = &buf;

    // Set backend cb
    get_surface_size = [](PainterImpl *self) {
        Size s;
        s.w = cairo_image_surface_get_width(self->surf);
        s.h = cairo_image_surface_get_height(self->surf);
        return s;
    };
    flush_surface = [](PainterImpl *self) {
        // Write the surface back to 
        PixBuffer       *buf  = self->target_info.bitmap;
        cairo_surface_t *surf = self->surf;

        uint32_t *src = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(surf));
        int     src_h = cairo_image_surface_get_height(surf);
        int     src_w = cairo_image_surface_get_width(surf);

        // Pitch shoule be 4 * w
        assert(src_w * 4 == cairo_image_surface_get_stride(surf));

        // Copy back
        if (buf->format() == PixFormat::RGBA32) {
            auto dst = buf->pixels<uint32_t>();
            for (int i = 0;i < src_w * src_h; i++) {
                dst[i] = argb_to_rgb32(src[i]);
            }
        }
        else {
            assert(!"Unsupport current format now");
        }
    };

    // Done
    initialize();
}
inline PainterImpl::~PainterImpl() {
    // Free pango first
    g_object_unref(layout);
    
    cairo_surface_destroy(surf);
    cairo_destroy(cr);
}
inline void PainterImpl::initialize() {
    // Make layout
    layout = pango_cairo_create_layout(cr);
    // Set width to 1.0f
    cairo_set_line_width(cr, 1.0f);

    // Get default value
    default_miter_limit = cairo_get_miter_limit(cr);
    default_join = cairo_get_line_join(cr);
    default_cap  = cairo_get_line_cap(cr);
}

inline void PainterImpl::notify_resize(int w, int h) {
    if (set_surface_size) {
        // With scaling support
        if (get_framebuffer_size) {
            auto s = get_framebuffer_size(this);
            w = s.w;
            h = s.h;
        }
        set_surface_size(this, w, h);
    }
}

inline void PainterImpl::apply_brush(const FRect &box) {
    if (!brush) {
        return;
    }

    switch (brush->type) {
    
    case BrushType::Solid : {
        cairo_set_source(cr, brush->pat);
        return;
    }
    case BrushType::Bitmap : {
        // Bitmap
        cairo_surface_t *surf;
        auto rect = brush_rect_to_abs(brush->mode, brush->rect, box);
        auto ret  = cairo_pattern_get_surface(brush->pat, &surf);
        float w, h;
        w = cairo_image_surface_get_width(surf);
        h = cairo_image_surface_get_height(surf);
        
        cairo_matrix_t mat;
        cairo_matrix_init_identity(&mat);
        cairo_matrix_translate(&mat, rect.x, rect.y);
        cairo_matrix_scale(&mat, rect.w / w, rect.h / h);
        cairo_matrix_invert(&mat);
        
        // Transform done
        cairo_pattern_set_matrix(brush->pat, &mat);
        cairo_set_source(cr, brush->pat);
        return;
    }
    case BrushType::RadialGradient : {
        [[fallthrough]];
    }
    case BrushType::LinearGradient : {
        // Rad 
        auto rect = brush_rect_to_abs(brush->mode, brush->rect, box);

        cairo_matrix_t mat;
        cairo_matrix_init_identity(&mat);
        cairo_matrix_translate(&mat, rect.x, rect.y);
        cairo_matrix_scale(&mat, rect.w, rect.h);
        cairo_matrix_invert(&mat);

        // Transform done
        cairo_pattern_set_matrix(brush->pat, &mat);
        cairo_set_source(cr, brush->pat);
        return;
    }
    default : {
        // Impossible
        abort();
    }

    }
}


// Painter
Painter::Painter() {
    priv = nullptr;
}
Painter::~Painter() {
    delete priv;
}
void Painter::Init() {
    BTK_ONCE(BTK_LOG("Cairo : This painter is experimental.\n"));

    if (mem_refcount != 0) {
        ++mem_refcount;
        return;
    }
    mem_surf = cairo_image_surface_create(
        CAIRO_FORMAT_A1,
        1,
        1
    );
    assert(mem_surf);
    mem_cr = cairo_create(mem_surf);
    assert(mem_cr);

    mem_refcount = 1;
}
void Painter::Shutdown() {
    --mem_refcount;
    if (mem_refcount == 0) {
        cairo_surface_destroy(mem_surf);
        cairo_destroy(mem_cr);
    }
}

// Method
void Painter::set_colorf(float r, float g, float b, float a) {
    cairo_set_source_rgba(priv->cr, r, g, b, a);
    priv->brush = nullptr;
}
void Painter::set_color(uint8_t r, uint8_t g ,uint8_t b, uint8_t a) {\
    cairo_set_source_rgba(
        priv->cr,
        r / 255.0,
        g / 255.0,
        b / 255.0,
        a / 255.0
    );
    priv->brush = nullptr;
}
void Painter::set_brush(const Brush &brush) {
    auto p = brush.priv;
    if (p == nullptr) {
        // No Brush
        set_color(Color::Black);
        return;
    }
    priv->brush = p;
}
void Painter::set_antialias(bool v) {
    if (v) {
        cairo_set_antialias(priv->cr, CAIRO_ANTIALIAS_BEST);
    }
    else {
        cairo_set_antialias(priv->cr, CAIRO_ANTIALIAS_NONE);
    }
}
void Painter::set_stroke_width(float w) {
    cairo_set_line_width(priv->cr, w);
}
void Painter::set_text_align(Alignment align) {
    priv->align = align;
}

void Painter::push_scissor(float x, float y, float w, float h) {
    cairo_save(priv->cr);
    cairo_rectangle(priv->cr, x, y, w, h);
    cairo_clip(priv->cr);
}
void Painter::pop_scissor() {
    cairo_restore(priv->cr);
}

// Draw
void Painter::draw_rect(float x, float y, float w, float h) {
    priv->apply_brush(FRect(x, y, w, h));

    cairo_rectangle(priv->cr, x, y, w, h);
    cairo_stroke(priv->cr);
}
void Painter::draw_line(float x1, float y1, float x2, float y2) {
    priv->apply_brush(FRect(
        min(x1, x2),
        min(x2, y2),
        std::abs(x2 - x1),
        std::abs(y2 - y1)
    ));

    cairo_move_to(priv->cr, x1, y1);
    cairo_line_to(priv->cr, x2, y2);
    cairo_stroke(priv->cr);
}
void Painter::draw_lines(const FPoint *fp, size_t n) {
    if (n < 2) {
        return;
    }
    for (size_t i = 0;i < n - 1; i++) {
        draw_line(fp[i].x, fp[i].y, fp[i+1].x, fp[i+1].y);
    }
}
void Painter::draw_rounded_rect(float x, float y, float w ,float h, float r) {
    priv->apply_brush(FRect(x, y, w, h));
    constexpr double deg = M_PI / 180;

    cairo_new_sub_path(priv->cr);
    cairo_arc(priv->cr, x + w - r, y + r, r, -90 * deg, 0 * deg);
    cairo_arc(priv->cr, x + w - r, y + h - r, r, 0 * deg, 90 * deg);
    cairo_arc(priv->cr, x + r, y + h - r, r, 90 * deg, 180 * deg);
    cairo_arc(priv->cr, x + r, y + r, r, 180 * deg, 270 * deg);
    cairo_close_path(priv->cr);
    cairo_stroke(priv->cr);
}
void Painter::draw_circle(float x, float y, float r) {
    priv->apply_brush(
        FRect(
            x - r,
            y - r,
            r * 2,
            r * 2
        )
    );

    cairo_new_sub_path(priv->cr);
    cairo_arc(priv->cr, x, y, r, 0.0, 2 * M_PI);
    cairo_stroke(priv->cr);
}
void Painter::draw_path(const PainterPath &p) {
    if (p.priv == nullptr) {
        return;
    }
    double x1, y1, x2, y2;
    cairo_append_path(priv->cr, p.priv->path);
    cairo_path_extents(priv->cr, &x1, &y1, &x2, &y2);

    // Apply brush
    priv->apply_brush(FRect(
        x1, y1,
        x2 - x1, y2 - y1
    ));

    cairo_stroke(priv->cr);
}
void Painter::draw_image(const Texture &t, const FRect *_dst, const FRect *_src) {
    cairo_surface_t *surf;
    auto pat = t.priv->pattern;
    auto ret = cairo_pattern_get_surface(pat, &surf);

    if (!pat || ret != CAIRO_STATUS_SUCCESS) {
        return;
    }

    FRect dst;
    FRect src;

    // float tex_w = cairo_image_surface_get_width(surf);
    // float tex_h = cairo_image_surface_get_height(surf);
    auto tex_size = t.size();
    float tex_w = tex_size.w;
    float tex_h = tex_size.h;


    if (_dst) {
        dst = *_dst;
    }
    else {
        auto [w, h] = priv->get_surface_size(priv);
        dst = FRect(0, 0, w, h);
    }

    if (_src) {
        src = *_src;
    }
    else {
        src = FRect(0, 0, tex_w, tex_h);
    }

    if (dst.empty() || src.empty()) {
        return;
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

    cairo_save(priv->cr);

    // cairo_set_source_surface(priv->cr, surf, 0, 0);
    // cairo_translate(priv->cr, tgt.x, tgt.y);
    // cairo_scale(priv->cr, tgt.w / tex_w, tgt.h / tex_h);
    cairo_matrix_t mat;
    cairo_matrix_init_translate(&mat, tgt.x, tgt.y);
    cairo_matrix_scale(&mat, tgt.w / tex_w, tgt.h / tex_h);
    cairo_matrix_invert(&mat); 
    cairo_pattern_set_matrix(pat, &mat);

    cairo_set_source(priv->cr, pat);

    cairo_rectangle(priv->cr, dst.x, dst.y, dst.w, dst.h);
    cairo_fill(priv->cr);

    cairo_matrix_init_identity(&mat);
    cairo_pattern_set_matrix(pat, &mat);

    cairo_restore(priv->cr);
}

// Fill
void Painter::fill_rect(float x, float y, float w, float h) {
    priv->apply_brush(FRect(x, y, w, h));
    cairo_rectangle(priv->cr, x, y, w, h);
    cairo_fill(priv->cr);
}
void Painter::fill_rounded_rect(float x, float y, float w ,float h, float r) {
    priv->apply_brush(FRect(x, y, w, h));
    constexpr double deg = M_PI / 180;

    cairo_new_sub_path(priv->cr);
    cairo_arc(priv->cr, x + w - r, y + r, r, -90 * deg, 0 * deg);
    cairo_arc(priv->cr, x + w - r, y + h - r, r, 0 * deg, 90 * deg);
    cairo_arc(priv->cr, x + r, y + h - r, r, 90 * deg, 180 * deg);
    cairo_arc(priv->cr, x + r, y + r, r, 180 * deg, 270 * deg);
    cairo_close_path(priv->cr);
    cairo_fill(priv->cr);
}
void Painter::fill_circle(float x, float y, float r) {
    priv->apply_brush(
        FRect(
            x - r,
            y - r,
            r * 2,
            r * 2
        )
    );

    cairo_new_sub_path(priv->cr);
    cairo_arc(priv->cr, x, y, r, 0.0, 2 * M_PI);
    cairo_fill(priv->cr);
}
void Painter::fill_path(const PainterPath &p) {
    if (p.priv == nullptr) {
        return;
    }
    double x1, y1, x2, y2;
    cairo_append_path(priv->cr, p.priv->path);
    cairo_path_extents(priv->cr, &x1, &y1, &x2, &y2);

    // Apply brush
    priv->apply_brush(FRect(
        x1, y1,
        x2 - x1, y2 - y1
    ));

    cairo_fill(priv->cr);
}

// Text
void Painter::set_font(const Font &f) {
    auto font = FONT_CAST(f.priv);

    // Set Font
    pango_layout_set_font_description(priv->layout, font);
}
void Painter::set_pen(const Pen *p) {
    if (p && p->priv) {
        auto pen = p->priv;
        cairo_set_dash(priv->cr, pen->dashes.data(), pen->dashes.size(), pen->dash_offset);
        cairo_set_miter_limit(priv->cr, pen->miter_limit);
        cairo_set_line_join(priv->cr, pen->join);
        cairo_set_line_cap(priv->cr, pen->cap);
    }
    else {
        // Reset to default
        cairo_set_dash(priv->cr, nullptr, 0, 0);
        cairo_set_line_cap(priv->cr, priv->default_cap);
        cairo_set_line_join(priv->cr, priv->default_join);
        cairo_set_miter_limit(priv->cr, priv->default_miter_limit);
    }
}
void Painter::draw_text(u8string_view txt, float x, float y) {

    // Set string
    pango_layout_set_text(priv->layout, txt.data(), txt.size());
    pango_cairo_update_layout(priv->cr, priv->layout);


    // Transform
    int w, h;
    pango_layout_get_pixel_size(priv->layout, &w, &h);
    layout_translate_by_align(priv->layout, priv->align, w, h, x, y);

    // Apply brush
    priv->apply_brush(FRect(x, y, w, h));

    // Draw it
    cairo_save(priv->cr);
    cairo_move_to(priv->cr, x, y);
    pango_cairo_show_layout(priv->cr, priv->layout);
    cairo_restore(priv->cr);
}
void Painter::draw_text(const TextLayout &layout, float x, float y) {
    auto lay = layout.priv->lazy_eval(priv->cr);

    // Transform
    int w, h;
    pango_layout_get_pixel_size(lay, &w, &h);
    layout_translate_by_align(lay, priv->align, w, h, x, y);

    // Apply brush
    priv->apply_brush(FRect(x, y, w, h));

    // Draw it
    cairo_save(priv->cr);
    cairo_move_to(priv->cr, x, y);
    pango_cairo_update_layout(priv->cr, lay);
    pango_cairo_show_layout(priv->cr, lay);
    cairo_restore(priv->cr);
}

// Texture
auto Painter::create_texture(PixFormat fmt, int w, int h) -> Texture {
    Texture t;
    t.priv = new TextureImpl(priv, fmt, w, h);
    return t;
}
auto Painter::create_texture(const PixBuffer &buf) -> Texture {
    auto tex = create_texture(buf.format(), buf.width(), buf.height());

    if (!tex.empty()) {
        tex.update(nullptr, buf.pixels(), buf.pitch());
    }
    return tex;
}


void Painter::begin() {
    // Begin frame, set scaling & current frame buffer object
    if (priv->window) {
        auto [win_w, win_h] = priv->window->size();
        auto [fb_w, fb_h] = priv->get_framebuffer_size(priv);

        cairo_save(priv->cr);
        cairo_scale(
            priv->cr,
            double(fb_w) / win_w,
            double(fb_h) / win_h
        );

        priv->set_surface_size(priv, fb_w, fb_h);
    }
    cairo_push_group(priv->cr);
}
void Painter::clear() {
    cairo_paint(priv->cr);
}
void Painter::end() {
    cairo_pop_group_to_source(priv->cr);
    cairo_paint(priv->cr);

    if (priv->window) {
        // Restore transform by dpi scaling
        cairo_restore(priv->cr);
    }
    
    if (priv->flush_surface) {
        priv->flush_surface(priv);
    }
    else {
        // Default flush op
        cairo_surface_flush(priv->surf);
    }

    //
    // if (priv->target_info.sw) {
    //     cairo_surface_write_to_png(priv->surf, "test.png");
    // }
}

// Transform
void Painter::translate(float x, float y) {
    cairo_translate(priv->cr, x, y);
}
void Painter::scale(float xs, float ys) {
    cairo_scale(priv->cr, xs, ys);
}
void Painter::rotate(float angle) {
    cairo_rotate(priv->cr, angle);
}
void Painter::reset_transform() {
    cairo_matrix_t mat;
    cairo_get_matrix(priv->cr, &mat);
    cairo_matrix_invert(&mat);
    cairo_transform(priv->cr, &mat);
}
void Painter::push_transform() {
    cairo_save(priv->cr);
}
void Painter::pop_transform() {
    cairo_restore(priv->cr);
}

// Get color 
auto Painter::color() const -> GLColor {
    double r, g, b, a;
    auto pattern = cairo_get_source(priv->cr);
    if (cairo_pattern_get_rgba(pattern, &r, &g, &b, &a) != CAIRO_STATUS_SUCCESS) {
        return Color::Black;
    }
    return GLColor(r, g, b, a);
}
auto Painter::brush() const -> Brush {
    Brush b;
    if (priv->brush) {
        // Has Brush, use it
        b.priv = COW_REFERENCE(priv->brush.get());
    }
    else {
        // Make a color brush
        b.set_color(color());
    }
    return b;
}


// Resize
void Painter::notify_resize(int w, int h) {
    priv->notify_resize(w, h);
}

Painter Painter::FromWindow(AbstractWindow *w) {
    Display *display = nullptr;
    Window   window  = 0;

    w->query_value(AbstractWindow::XDisplay, &display);
    w->query_value(AbstractWindow::XWindow , &window);

    if (display && window) {
        auto priv = new PainterImpl(display, window);
        priv->window = w;
        return Painter(priv);
    }
    BTK_THROW(std::runtime_error("Unsupported backend"));
}
Painter Painter::FromXlib(void *dpy, void *xwin) {
    Painter p;
    p.priv = new PainterImpl(
        reinterpret_cast<::Display*>(dpy),
        reinterpret_cast<::Window>(xwin)
    );
    return p;
}
Painter Painter::FromPixBuffer(PixBuffer &buf) {
    Painter p;
    p.priv = new PainterImpl(buf);
    return p;
}
bool    Painter::HasFeature(PainterFeature f) {
    // Currently support all
    return true;
}

// Operator
void Painter::operator =(Painter &&p) {
    if (&p != this) {
        delete priv;
        priv = p.priv;
        p.priv = nullptr;
    }
}

// TextLayout
PangoLayout *TextLayoutImpl::lazy_eval(cairo_t *c) {
    // Default args
    if (c == nullptr && cr == nullptr) {
        c = mem_cr;
    }
    else if (cr != nullptr) {
        c = cr;
    }

    if (c != cr || layout.empty()) {
        cr = c;
        // Make new one
        GPointer<PangoLayout> lay(
            pango_cairo_create_layout(c)
        );

        // Set to new
        layout = lay;

        // Dirty way to get the font ptr
        static_assert(sizeof(Font) == sizeof(void*));
        auto f = *((PangoFontDescription**)&font);
        assert(f);

        pango_layout_set_font_description(layout.get(), f);
        pango_layout_set_text(layout.get(), text.c_str(), text.size());
        pango_cairo_update_layout(cr, layout.get());
    }
    return layout.get();
}


// TODO : Still has a lot of bug here
COW_IMPL(TextLayout);
TextLayout::TextLayout() {
    priv = nullptr;
}

void TextLayout::set_text(u8string_view txt) {
    begin_mut();
    priv->layout.reset();

    priv->text = txt;
    priv->text_len = -1;
}
void TextLayout::set_font(const Font &f) {
    begin_mut();
    priv->layout.reset();

    priv->font = f;
}
FSize TextLayout::size() const {
    if (priv) {
        auto lay = priv->lazy_eval();
        // Get it
        int w, h;
        pango_layout_get_pixel_size(lay, &w, &h);
        return FSize(w, h);
    }
    return FSize(0, 0);
}
size_t TextLayout::line() const {
    if (priv) {
        auto lay = priv->lazy_eval();
        return pango_layout_get_line_count(lay);
    }
    return 0;
}
bool TextLayout::hit_test(float x, float y, TextHitResult *result) const {
    if (priv) {
        auto lay = priv->lazy_eval();


        // Use pango to get the cursor pos
        int x_pos, y_pos;
        int index;
        int trailing;
        bool inside = pango_layout_xy_to_index(lay, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
        auto ch = pango_layout_get_text(lay);
        if (result) {
            // Change this index to logical
            result->text = Utf8Locate(ch, ch + index);
            result->length = 1;
            result->inside = inside;
            result->trailing = trailing;
            // Get the rect
            PangoRectangle rect;
            pango_layout_index_to_pos(lay, index, &rect);
            result->box.x = float(rect.x) / float(PANGO_SCALE);
            result->box.y = float(rect.y) / float(PANGO_SCALE);
            result->box.w = float(rect.width) / float(PANGO_SCALE);
            result->box.h = float(rect.height) / float(PANGO_SCALE);
        }
        return true;
    }
    return false;
}
// bool TextLayout::hit_test_pos(size_t pos, float org_x, float org_y, TextHitResult *result) const {
//     if (priv) {
//         auto lay = priv->lazy_eval();
//         auto iter = pango_layout_get_iter(lay);

//         size_t n = 0;
//         bool matched = false;
//         PangoRectangle rect;

//         do {
//             if (n != pos) {
//                 n += 1;
//                 continue;
//             }
//             // Matched
//             pango_layout_iter_get_char_extents(iter, &rect);
//             matched = true;
//             break;
//         }
//         while (pango_layout_iter_next_char(iter));

//         pango_layout_iter_free(iter);
//         if (!matched) {
//             // ?? I didnot konwn what condtional the directwrite just abort now
//             BTK_THROW(std::runtime_error("Bad pos"));
//         }
//         if (result) {
//             result->inside = false;
//             result->trailing = false;

//             result->text = n;
//             result->length = 1;

//             result->box.x = rect.x / PANGO_SCALE + org_x;
//             result->box.y = rect.x / PANGO_SCALE + org_y;
//             result->box.w = rect.width / PANGO_SCALE;
//             result->box.h = rect.height / PANGO_SCALE;
//         }
//         return true;
//     }
//     return false;
// }
bool TextLayout::hit_test_range(size_t pos, size_t len, float org_x, float org_y, TextHitResults *res) const {
    if (priv) {
        PangoRectangle rectangle;
        PangoLayoutLine *cur_line;
        gboolean has_next;
        FRect    box;
        auto lay = priv->lazy_eval();

        if (!res) {
            return true;
        }
        res->resize(0);

        if (priv->text_len == -1) {
            priv->text_len = priv->text.length();
        }
        if (pos + len > priv->text_len) {
            if (pos >= priv->text_len) {
                // No char
                return true;
            }
            len = priv->text_len - len;
        }

        // Begin iteration
        auto iter = pango_layout_get_iter(lay);
        for (size_t i = 0;i < pos; i += 1) {
            has_next = pango_layout_iter_next_char(iter);
            assert(has_next);
        }
        // Get first char
        pango_layout_index_to_pos(
            lay,
            pango_layout_iter_get_index(iter),
            &rectangle
        );
        cur_line = pango_layout_iter_get_line_readonly(iter);
        box.x = rectangle.x / PANGO_SCALE;
        box.y = rectangle.y / PANGO_SCALE;
        box.h = rectangle.height / PANGO_SCALE;
        box.w = 0;

        size_t i;
        for (i = 0; i < len; i ++) {
            // Increase the box
            pango_layout_index_to_pos(
                lay,
                pango_layout_iter_get_index(iter),
                &rectangle
            );

            box.y = min(box.y, float(rectangle.y) / PANGO_SCALE);
            box.h = max(box.h, float(rectangle.height / PANGO_SCALE));
            box.w = max(box.w, float((rectangle.x + rectangle.width) / PANGO_SCALE) - box.x);

            // Next char
            has_next = pango_layout_iter_next_char(iter);
            if (!has_next) {
                i += 1;
                break;
            }

            auto line = pango_layout_iter_get_line_readonly(iter);
            if (line != cur_line) {
                // Switch to next line
                cur_line = line;
                box.x = rectangle.x / PANGO_SCALE;
                box.y = rectangle.y / PANGO_SCALE;
                box.h = rectangle.height / PANGO_SCALE;
                box.w = 0;

                // Commit
                TextHitResult result;
                result.text = pos;
                result.length = i;
                result.inside = true;
                result.trailing = true;
                result.box = box;
                result.box.x += org_x;
                result.box.y += org_y;

                pos += i;

                res->push_back(result);
            }
        }
        pango_layout_iter_free(iter);

        // Commit
        TextHitResult result;
        result.text = pos;
        result.length = i;
        result.inside = true;
        result.trailing = true;
        result.box = box;
        result.box.x += org_x;
        result.box.y += org_y;

        res->push_back(result);

#if    !defined(NDEBUG)
        BTK_LOG("TextLayout: string = '%s'\n", pango_layout_get_text(lay));
        BTK_LOG("    Input params pos = %d, len = %d, org_x = %f, org_y = %f\n", int(pos), int(len), org_x, org_y);
        int num = 0;
        for (auto &result : *res) {
            BTK_LOG("    Result[%d]: box = {%f, %f, %f, %f} text = %d, len = %d\n",
                num,
                result.box.x,
                result.box.y,
                result.box.w,
                result.box.h,
                int(result.text),
                int(result.length) 
            );
        }
#endif
        return true;
    }
    return false;
}
bool TextLayout::line_metrics(TextLineMetricsList *metrics) const {
    if (priv) {
        auto lay = priv->lazy_eval();
        if (metrics) {
            metrics->clear();

            int line_count = pango_layout_get_line_count(lay);
            for (int i = 0; i < line_count; i++) {
                PangoLayoutLine *line = pango_layout_get_line(lay, i);
                PangoRectangle rect;
                pango_layout_line_get_extents(line, nullptr, &rect);

                TextLineMetrics line_metrics;
                line_metrics.height = (float) rect.height / PANGO_SCALE;
                line_metrics.baseline = (float) pango_layout_get_baseline(lay) / PANGO_SCALE;

                metrics->push_back(line_metrics);
            }

            return true;
        }
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
    cairo_pattern_destroy(priv->pat);
    priv->pat = cairo_pattern_create_rgba(
        c.r,
        c.g,
        c.b,
        c.a
    );
    priv->type = BrushType::Solid;
}
void Brush::set_gradient(const LinearGradient& c) {
    begin_mut();
    cairo_pattern_destroy(priv->pat);
    // Start point and end point should be normalized from [0 => 1.0]
    auto start = c.start_point();
    auto end   = c.end_point();
    priv->pat = cairo_pattern_create_linear(
        start.x,
        start.y,
        end.x,
        end.x
    );
    priv->type = BrushType::LinearGradient;
    // priv->data = c;

    for (auto [offset, color] : c.stops()) {
        cairo_pattern_add_color_stop_rgba(
            priv->pat,
            offset,
            color.r,
            color.g,
            color.b,
            color.a
        );
    }
}
void Brush::set_gradient(const RadialGradient &c) {
    begin_mut();
    cairo_pattern_destroy(priv->pat);
    priv->type = BrushType::RadialGradient;
    // priv->data = c;

    // TODO .... Find the way to slove radius_x and radius_y
    // All elems should be normalized from [0 => 1.0]
    auto center = c.center_point();
    auto offset = c.origin_offset();

    priv->pat = cairo_pattern_create_radial(
        center.x + offset.x,
        center.y + offset.y,
        0.0, // Emm, maybe better way
        center.x,
        center.y,
        c.radius_x()
    );
    // Add stops
    for (auto [offset, color] : c.stops()) {
        cairo_pattern_add_color_stop_rgba(
            priv->pat,
            offset,
            color.r,
            color.g,
            color.b,
            color.a
        );
    }
}
void Brush::set_image(const PixBuffer &buf) {
    begin_mut();
    cairo_pattern_destroy(priv->pat);

    auto surf = cairo_image_surface_create(
        btk_fmt_to_cr(buf.format()),
        buf.width(),
        buf.height()
    );

    if (buf.format() == PixFormat::RGBA32) {
        // Is ABGR32
        // Need cvt
        auto dst = cairo_image_surface_get_data(surf);
        auto src = buf.pixels<uint32_t>();

        for (int i = 0;i < buf.width() * buf.height(); i ++) {
            (uint32_t&)dst[i * 4] = rgba32_to_argb(src[i]);
        }

        cairo_surface_mark_dirty(surf);
    }

    priv->pat = cairo_pattern_create_for_surface(surf);
    priv->type = BrushType::Bitmap;

    cairo_surface_destroy(surf);
}
GLColor   Brush::color() const {
    if (priv && priv->type == BrushType::Solid) {
        double r, g, b, a;
        if (cairo_pattern_get_rgba(priv->pat, &r, &g, &b, &a) == CAIRO_STATUS_SUCCESS) {
            return GLColor(r, g, b, a);
        }
    }
    return Color::Black;
}
BrushType Brush::type() const {
    if (priv) {
        return priv->type;
    }
    return BrushType::Solid;
}

// Texture
TextureImpl::TextureImpl(PainterImpl *painter, PixFormat fmt, int w, int h) {

#if 0
    // Try native
    if (painter->target_opt.xlib && painter->target_info.xlib.shm) {
        // XLIB try Shm
        if (xlib_create(painter, fmt, w, h)) {
            return;
        }
    }
#endif

    if (!sw_create(painter, fmt, w, h)) {
        BTK_THROW(std::runtime_error("Painter cannot create texure\n"));
    }
}
TextureImpl::~TextureImpl() {
    std::invoke(destroy, this);
}

bool TextureImpl::sw_create(PainterImpl *, PixFormat fmt, int w, int h) {
    // Try sw
    cairo_format_t f;
    switch (fmt) {
        case PixFormat::RGBA32 : {
            f = CAIRO_FORMAT_ARGB32;
            break;
        }
        case PixFormat::RGB24  : {
            f = CAIRO_FORMAT_RGB24;
            break;
        }
        default : {
            BTK_LOG("SW Unsupported texture format\n");
            return false;
        }
    }
    surface = cairo_image_surface_create(f, w, h);
    pattern = cairo_pattern_create_for_surface(surface);

    get_size = &TextureImpl::sw_get_size;
    destroy  = &TextureImpl::sw_destroy;
    update   = &TextureImpl::sw_update;
    return true;
}
Size TextureImpl::sw_get_size() {
    int w, h;
    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);

    return {w, h};
}
void TextureImpl::sw_update(const Rect *where, cpointer_t data, uint32_t pitch) {
    cairo_surface_flush(surface);

    // Mod
    cairo_format_t fmt;
    int w, h, stride, byte;
    stride = cairo_image_surface_get_stride(surface);
    fmt = cairo_image_surface_get_format(surface);
    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);

    // Get Rect
    Rect rect;
    if (where == nullptr) {
        rect.x = 0;
        rect.y = 0;
        rect.w = w;
        rect.h = h;
    }
    else {
        rect = *where;
    }
    // Write pixels
    auto dst = cairo_image_surface_get_data(surface);
    auto src = static_cast<const uint8_t*>(data);


    // Copy pixels
    // Seek to current x, y
    dst += rect.x * 4;
    dst += rect.y * stride;

    // User byte
    byte = pitch / rect.w;

    assert((byte == 3 && fmt == CAIRO_FORMAT_RGB24) || (byte == 4 && fmt == CAIRO_FORMAT_ARGB32));

    if (fmt == CAIRO_FORMAT_ARGB32) {
        for (int y = 0; y < h; y++) {
            // Copy line
            for (int x = 0; x < w; x++) {
                (uint32_t&)dst[x * 4] = rgba32_to_argb((uint32_t&)src[x * 4]);
            }

            src += pitch;
            dst += stride;
        }
    }
    else {
        for (int y = 0; y < h; y++) {
            // Copy line
            for (int x = 0; x < w; x++) {
                dst[x * 4 + 0] = src[x * byte + 0];
                dst[x * 4 + 1] = src[x * byte + 1];
                dst[x * 4 + 2] = src[x * byte + 2];
            }

            src += pitch;
            dst += stride;
        }
    }

    cairo_surface_mark_dirty_rectangle(surface, rect.x, rect.y, rect.w, rect.h);
}
void TextureImpl::sw_destroy() {
    cairo_surface_destroy(surface);
    cairo_pattern_destroy(pattern);
}

#if 0
bool TextureImpl::xlib_create(PainterImpl *painter, PixFormat fmt, int w, int h) {
    XWindowAttributes attrs;
    XVisualInfo       vinfo;
    Display *dpy = painter->target_info.xlib.display;
    Window   win = painter->target_info.xlib.window;

    if (XShmPixmapFormat(dpy) != ZPixmap) {
        BTK_LOG("[Xlib::Shm] PixFormat unsupport ZPixmap, give up it\n");
        return false;
    }

    int ret;
    
    switch (fmt) {
        case PixFormat::RGBA32 : {
            ret = XMatchVisualInfo(
                dpy,
                DefaultScreen(dpy),
                32,
                TrueColor,
                &vinfo
            );
            break;
        }
        default : {
            BTK_LOG("[Xlib::Shm] SHM Unsupported texture format\n");
            return false;
        } 
    }

    if (ret == 0) {
        return false;
    }
    if (XGetWindowAttributes(dpy, win, &attrs) == 0) {
        return false;
    }
    image = XShmCreateImage(
        dpy,
        vinfo.visual,
        32,
        ZPixmap,
        nullptr,
        &shminfo,
        w,
        h
    );
    if (!image) {
        return false;
    }
    shminfo.shmid = shmget(
        IPC_PRIVATE,
        image->bytes_per_line * image->height,
        IPC_CREAT | 0777);

    shminfo.shmaddr = image->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
    shminfo.readOnly = False;
    
    if (XShmAttach(dpy, &shminfo) == 0) {
        return false;
    }
    auto pixmap = XShmCreatePixmap(
        dpy,
        win, 
        image->data, 
        &shminfo,
        w,
        h,
        32
    );

    if (pixmap == 0) {
        // Error
        BTK_LOG("[Xlib::Shm] XShmCreatePixmap failed\n");
        return false;
    }

    XGCValues v = {};
    gc = XCreateGC(dpy, win, 0, &v);

    surface = cairo_xlib_surface_create_for_bitmap(dpy, pixmap, XDefaultScreenOfDisplay(dpy), w, h);
    pattern = cairo_pattern_create_for_surface(surface);

    get_size = &TextureImpl::xlib_get_size;
    destroy = &TextureImpl::xlib_destroy;
    update  = &TextureImpl::xlib_update;

    BTK_LOG("[Xlib::Shm] Texture created on shared memory\n");

    return true;
}
void TextureImpl::xlib_destroy() {
    auto pixmap = cairo_xlib_surface_get_drawable(surface);
    auto dpy = cairo_xlib_surface_get_display(surface);

    XDestroyImage(image);
    XFreePixmap(dpy, pixmap);

    // Detach shm
    XShmDetach(dpy, &shminfo);

    // Release shm
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, nullptr);

    // Release GC
    XFreeGC(dpy, gc);
}
Size TextureImpl::xlib_get_size() {
    int width = cairo_xlib_surface_get_width(surface);
    int height = cairo_xlib_surface_get_width(surface);
    
    return Size(width, height);
}
void TextureImpl::xlib_update(const Rect *where, cpointer_t data, uint32_t pitch) {
    auto pixmap = cairo_xlib_surface_get_drawable(surface);
    auto dpy = cairo_xlib_surface_get_display(surface);
    auto [w, h] = xlib_get_size();

    assert(where == nullptr);

    Btk_memcpy(shminfo.shmaddr, data, pitch * h);
    
    int ret = XShmPutImage(
        dpy, 
        pixmap, 
        gc, 
        image,
        0, 
        0, 
        0, 
        0,
        w,
        h,
        false 
    );

    if (ret == 0) {
        BTK_LOG("[Xlib::Shm] XShmPutImage failed\n");
        abort();
    }

    cairo_surface_mark_dirty(surface);
}
#endif

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
void Texture::clear() {
    delete priv;
    priv = nullptr;
}
bool Texture::empty() const {
    return priv == nullptr;
}
Size Texture::size() const {
    if (priv) {
        return std::invoke(priv->get_size, priv);
    }
    return Size(0, 0);
}
void Texture::update(const Rect *where, cpointer_t data, uint32_t pitch) {
    if (priv) {
        return std::invoke(priv->update, priv, where, data, pitch);
    }
}
Texture &Texture::operator =(Texture &&t) {
    if (&t != this) {
        priv = t.priv;
        t.priv = nullptr;
    }
    return *this;
}
Texture &Texture::operator =(std::nullptr_t p) {
    clear();
    return *this;
}



// Font
Font::Font() {
    priv = nullptr;
}
Font::Font(u8string_view family, float size) {
    auto &ref = FONT_CAST(priv);
    auto pattern = u8string::format("%s %dpx", u8string(family).c_str(), int(std::round(size)));
    ref = pango_font_description_from_string(pattern.c_str());

    // pango_font_description_set_absolute_size(ref, size * PANGO_SCALE);
}
Font::Font(const Font &f) {
    FONT_CAST(priv) = pango_font_description_copy(FONT_CAST(f.priv));    
}
Font::Font(Font &&f) {
    swap(f);
}
Font::~Font() {
    pango_font_description_free(FONT_CAST(priv));
}

void Font::swap(Font &f) {
    priv = f.priv;
    f.priv = nullptr;
}
Font &Font::operator =(const Font &f) {
    auto &fref = FONT_CAST(f);
    auto &ref = FONT_CAST(priv);

    pango_font_description_free(ref);
    ref = pango_font_description_copy(fref);
    return *this;
}
Font &Font::operator =(Font &&f) {
    swap(f);
    return *this;
}

float Font::size() const {
    return pango_font_description_get_size(FONT_CAST(priv)) / PANGO_SCALE;
}

void  Font::set_size(float size) {
    pango_font_description_set_absolute_size(FONT_CAST(priv), size * PANGO_SCALE);
}
void  Font::set_family(u8string_view family) {
    pango_font_description_set_family(FONT_CAST(priv), u8string(family).c_str());
}
void  Font::set_bold(bool v) {
    pango_font_description_set_weight(FONT_CAST(priv), v ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
}
void  Font::set_italic(bool v) {
    pango_font_description_set_style(FONT_CAST(priv), v ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
}

auto  Font::ListFamily() -> StringList {
    GPointer<PangoLayout> layout;
    layout = pango_cairo_create_layout(mem_cr);
    auto ctxt = pango_layout_get_context(layout.get());
    auto map  = pango_context_get_font_map(ctxt);

    PangoFontFamily ** familys;
    int                nfaimilys;
    pango_font_map_list_families(map, &familys, &nfaimilys);

    StringList result;
    for (int i = 0;i < nfaimilys; i++) {
        result.push_back(pango_font_family_get_name(familys[i]));
    }

    g_free(familys);
    return result;
}

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
    if (priv == nullptr) {
        // First time
        priv = new PainterPathImpl;
    }
    cairo_save(priv->cr);
    cairo_new_path(priv->cr);
    if (priv->path) {
        // Already has path
        cairo_append_path(priv->cr, priv->path);
        cairo_path_destroy(priv->path);

        priv->path = nullptr;
    }
}
void PainterPath::close() {
    priv->path = cairo_copy_path(priv->cr);

    cairo_restore(priv->cr);
}
void PainterPath::move_to(float x, float y) {
    cairo_move_to(priv->cr, x, y);
    priv->last_x = x;
    priv->last_y = y;
}
void PainterPath::line_to(float x, float y) {
    cairo_line_to(priv->cr, x, y);
    priv->last_x = x;
    priv->last_y = y;
}
void PainterPath::quad_to(float x1, float y1, float x2, float y2) {
    fallback_painter_quad_to(
        this,
        priv->last_x,
        priv->last_y,
        x1,
        y1,
        x2,
        y2
    );
}
void PainterPath::arc_to(float x1, float y1, float x2, float y2, float radius) {
    // cairo_arc(cairo_t *cr, double xc, double yc, double radius, double angle1, double angle2)
    fallback_painter_arc_to(
        this,
        priv->last_x,
        priv->last_y,
        x1,
        y1,
        x2,
        y2,
        radius
    );
}
void PainterPath::bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) {
    cairo_curve_to(priv->cr, x1, y1, x2, y2, x3, y3);
    priv->last_x = x3;
    priv->last_y = y3;
}
void PainterPath::close_path() {
    cairo_close_path(priv->cr);
}
void PainterPath::set_winding(PathWinding winding) {
    BTK_UNUSED(winding);
}
FRect PainterPath::bounding_box() const {
    double x1, y1, x2, y2;
    cairo_new_path(priv->cr);
    cairo_append_path(priv->cr, priv->path);
    cairo_path_extents(priv->cr, &x1, &y1, &x2, &y2);
    return FRect(x1, y1, x2 - x1, y2 - y1);
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
void Pen::set_dash_pattern(const float *pat, size_t n) {
    begin_mut();
    priv->dashes.clear();

    for (size_t i = 0;i < n; i++) {
        priv->dashes.push_back(pat[i]);
    }
}
void Pen::set_dash_style(DashStyle style) {
    begin_mut();
    priv->dashes.clear();

    // FIXME : Add right dash pattern
    switch (style) {
        case DashStyle::DashDotDot : {
            priv->dashes = {2.0, 1.0, 1.00};
            break;
        }
        case DashStyle::DashDot : {
            priv->dashes = {2.0, 1.0};
            break;
        }
        case DashStyle::Dot : {
            priv->dashes = {1.0, 1.0};
            break;
        }
    }
}
void Pen::set_dash_offset(float offset) {
    begin_mut();
    priv->dash_offset = offset;
}
void Pen::set_line_join(LineJoin j) {
    begin_mut();
    switch (j) {
        case LineJoin::Miter : priv->join = CAIRO_LINE_JOIN_MITER; break;
        case LineJoin::Bevel : priv->join = CAIRO_LINE_JOIN_BEVEL; break;
        case LineJoin::Round : priv->join = CAIRO_LINE_JOIN_ROUND; break;
    }
}
void Pen::set_line_cap(LineCap c) {
    begin_mut();
    switch (c) {
        case LineCap::Flat   : priv->cap = CAIRO_LINE_CAP_BUTT; break;
        case LineCap::Round  : priv->cap = CAIRO_LINE_CAP_ROUND; break;
        case LineCap::Square : priv->cap = CAIRO_LINE_CAP_SQUARE; break;
    }
}
void Pen::set_miter_limit(float limit) {
    begin_mut();
    priv->miter_limit = limit;
}


BTK_NS_END

#undef FONT_CAST
#endif
*/