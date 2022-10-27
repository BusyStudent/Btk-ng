#include "build.hpp"

#include <Btk/widgets/view.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

// Label
Label::Label(Widget *wi, u8string_view txt) : Widget(wi) {
    _layout.set_font(font());
    _layout.set_text(txt);

    // Try measure text
    auto size = size_hint();
    if (size.is_valid()) {
        resize(size.w, size.h);
    }
}
Label::~Label() {}

void Label::set_text(u8string_view txt) {
    _layout.set_text(txt);
}
bool Label::paint_event(PaintEvent &) {
    auto border = this->rect().cast<float>();
    auto style = this->style();
    auto &painter = this->painter();

    painter.set_font(font());
    painter.set_text_align(_align);
    painter.set_color(style->text);

    painter.push_scissor(border);
    painter.draw_text(
        _layout,
        border.x,
        border.y + border.h / 2
    );
    painter.pop_scissor();

    return true;
}
bool Label::change_event(Event &event) {
    if (event.type() == Event::FontChanged) {
        _layout.set_font(font());
    }
    return true;
}
Size Label::size_hint() const {
    auto size = _layout.size();
    if (size.is_valid()) {
        // Add margin
        return Size(size.w + 2, size.h + 2);
    }
    return size;
}

// ImageView
ImageView::ImageView(Widget *p, const PixBuffer *image) : Widget(p) {
    if (image != nullptr) {
        set_image(*image);
    }
}
ImageView::~ImageView() {}

void ImageView::set_image(const PixBuffer &img) {
    pixbuf = img;
    dirty = true;
    repaint();
}
void ImageView::set_keep_aspect_ratio(bool keep) {
    keep_aspect = keep;
    repaint();
}
bool ImageView::paint_event(PaintEvent &event) {
    BTK_UNUSED(event);
    if (dirty || texture.empty()) {
        dirty = false;

        // Update texture
        texture.clear();
        if (!pixbuf.empty()) {
            texture = painter().create_texture(pixbuf);
        }
    }
    if (!texture.empty()) {
        FRect dst;
        if (keep_aspect) {
            // Keep aspect ratio
            FSize size = rect().size();
            FSize img_size = pixbuf.size();

            if (img_size.w * size.h > img_size.h * size.w) {
                dst.w = size.w;
                dst.h = img_size.h * size.w / img_size.w;
            }
            else {
                dst.h = size.h;
                dst.w = img_size.w * size.h / img_size.h;
            }
            dst.x = rect().x + (size.w - dst.w) / 2;
            dst.y = rect().y + (size.h - dst.h) / 2;
        }
        else{
            dst = this->rect().cast<float>();
        }

        painter().draw_image(texture, &dst, nullptr);
    }

    return true;
}
Size ImageView::size_hint() const {
    if (!pixbuf.empty()) {
        auto [x, y] = window_dpi();
        return Size(
            PixelToIndependent(pixbuf.width(), x),
            PixelToIndependent(pixbuf.height(), y)
        );
    }
    return Size(0, 0);
}

BTK_NS_END