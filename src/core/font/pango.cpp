#include "build.hpp"
#include "common/utils.hpp"
#include "common/device_resource.hpp"

#include <pango/pangocairo.h>
#include <Btk/painter.hpp>
#include <Btk/object.hpp>

#define FONT_CAST(ptr) ((PangoFontDescription*&)ptr)

BTK_NS_BEGIN

static cairo_t *mem_cr = nullptr;
static int mem_refcount = 0;

class TextLayoutImpl : public Refable<TextLayoutImpl>, public PaintResourceManager {
    public:
        GPointer<PangoLayout> create_layout(cairo_t *cr);
        PangoLayout          *lazy_eval();

        GPointer<PangoLayout> layout;
        PainterPath           path;
        Font                  font;
        u8string              text;
        int64_t               text_len = -1;
};


// Font Impl
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
bool  Font::empty() const {
    return priv == nullptr;
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

bool  Font::native_handle(FontHandle what, void *out) const {
    if (what == FontHandle::PangoFontDescription) {
        *static_cast<PangoFontDescription**>(out) = FONT_CAST(priv);
        return true;
    }
    return false;
}

void  Font::Init() {
    // Add ref
    mem_refcount ++;
    if (!mem_cr) {
        auto surf = cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
        mem_cr = cairo_create(surf);
        cairo_surface_destroy(surf);
        return;
    }    
}
void  Font::Shutdown() {
    --mem_refcount;
    if (mem_refcount == 0) {
        cairo_destroy(mem_cr);
        mem_cr = nullptr;
    }
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

// TextLayout
PangoLayout *TextLayoutImpl::lazy_eval() {
    if (layout.empty()) {
        layout = create_layout(mem_cr);
    }
    return layout.get();
}
GPointer<PangoLayout> TextLayoutImpl::create_layout(cairo_t *cr) {
    // Make new one
    GPointer<PangoLayout> lay(
        pango_cairo_create_layout(cr)
    );

    // Dirty way to get the font ptr
    static_assert(sizeof(Font) == sizeof(void*));
    auto f = *((PangoFontDescription**)&font);
    assert(f);

    pango_layout_set_font_description(lay.get(), f);
    pango_layout_set_text(lay.get(), text.c_str(), text.size());
    pango_cairo_update_layout(cr, lay.get());

    return lay;
}


// TODO : Still has a lot of bug here
COW_BASIC_IMPL(TextLayout);

TextLayout::TextLayout() {
    priv = nullptr;
}
void TextLayout::begin_mut() {
    COW_MUT(priv);
    priv->reset_manager();
    priv->layout.reset();
    priv->path.clear();
}
void TextLayout::set_text(u8string_view txt) {
    begin_mut();

    priv->text = txt;
    priv->text_len = -1;
}
void TextLayout::set_font(const Font &f) {
    begin_mut();

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
PainterPath TextLayout::outline(float dpi) const {
    if (!priv) {
        return PainterPath();
    }
    if (!priv->path.empty()) {
        return priv->path;
    }
    auto lay = priv->lazy_eval();
    if (!lay) {
        return PainterPath();
    }

    // Get ext
    cairo_save(mem_cr);
    pango_cairo_layout_path(mem_cr, lay);

    // Get path
    auto cr_path = cairo_copy_path(mem_cr);
    // Convert
    priv->path.open();
    for (int i = 0; i < cr_path->num_data; i += cr_path->data[i].header.length) {
        auto data = &cr_path->data[i];
        switch (data->header.type) {
            case CAIRO_PATH_MOVE_TO:
                priv->path.move_to(data[1].point.x, data[1].point.y);
                break;
            case CAIRO_PATH_LINE_TO:
                priv->path.line_to(data[1].point.x, data[1].point.y);
                break;
            case CAIRO_PATH_CURVE_TO:
                priv->path.bezier_to(data[1].point.x, data[1].point.y,
                                    data[2].point.x, data[2].point.y,
                                    data[3].point.x, data[3].point.y);
                break;
            case CAIRO_PATH_CLOSE_PATH:
                priv->path.close_path();
                break;
        }
    }
    priv->path.close();

    cairo_path_destroy(cr_path);
    cairo_restore(mem_cr);

    return priv->path;
}
Font TextLayout::font() const {
    if (priv) {
        return priv->font;
    }
    return Font();
}
bool TextLayout::rasterize(float dpi, std::vector<PixBuffer> *bitmaps, std::vector<Rect> *rect) const {
    // Render to bitmap
    auto logical_size = size();

    // Scale by dpi
    float xscale = dpi / 96.0f;
    float yscale = dpi / 96.0f;

    Size phy_size;
    phy_size.w = std::round(logical_size.w) * xscale;
    phy_size.h = std::round(logical_size.h) * yscale;

    // Create pixbuffer
    PixBuffer buffer(PixFormat::Gray8, phy_size.w, phy_size.h);
    cairo_surface_t *buffer_surf = cairo_image_surface_create(
        CAIRO_FORMAT_A8,
        buffer.width(),
        buffer.height()
    );
    cairo_t *buffer_cr = cairo_create(buffer_surf);

    // Make lay
    auto lay = priv->create_layout(buffer_cr);

    // Begin paint
    cairo_scale(buffer_cr, xscale, yscale);

    // Draw
    cairo_move_to(buffer_cr, 0, 0);
    cairo_set_source_rgba(buffer_cr, 1.0, 1.0, 1.0, 1.0);

    pango_cairo_update_layout(buffer_cr, lay.get());
    pango_cairo_show_layout(buffer_cr, lay.get());

    // Done
    cairo_destroy(buffer_cr);
    cairo_surface_flush(buffer_surf);

    // Write back
    int surf_pitch = cairo_image_surface_get_stride(buffer_surf);
    int surf_w = cairo_image_surface_get_width(buffer_surf);
    int surf_h = cairo_image_surface_get_height(buffer_surf);
    uint8_t *pix = (uint8_t*) cairo_image_surface_get_data(buffer_surf);
    uint8_t *dst = buffer.pixels<uint8_t>();

    for (int y = 0; y < surf_h; y++) {
        Btk_memcpy(dst + y * buffer.pitch(), pix + y * surf_pitch, surf_w);
    }

    cairo_surface_destroy(buffer_surf);

    if (bitmaps) {
        bitmaps->clear();
        bitmaps->push_back(buffer);
    }
    if (rect) {
        rect->clear();
        rect->push_back(FRect(0, 0, phy_size.w, phy_size.h));
    }

    return true;
}

void TextLayout::bind_device_resource(void *key, PaintResource *resource) const {
    priv->add_resource(key, resource);
}
auto TextLayout::query_device_resource(void *key) const -> PaintResource * {
    return priv->get_resource(key);
}

BTK_NS_END