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
ImageView::~ImageView() {
    if (timer) {
        del_timer(timer);
    }
}

void ImageView::set_image(const PixBuffer &img) {
    image.clear();
    pixbuf = img;
    dirty = true;
    frame = 0;
    delay = 0;

    if (timer) {
        del_timer(timer);
        timer = 0;
    }

    repaint();
}
void ImageView::set_image(const Image &img) {
    pixbuf.clear();
    image = img;
    dirty = true;
    frame = 0;
    delay = 0;

    if (!image.empty()) {
        if (!image.read_frame(0, pixbuf, &delay)) {
            image.clear();
            repaint();
            return;
        }
        if (delay != -1) {
            timer = add_timer(delay);
        }
    }

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

        // Update texture, try reuse it
        if (!pixbuf.empty() && !texture.empty() && pixbuf.size() == texture.size()) {
            texture.update(nullptr, pixbuf.pixels(), pixbuf.pitch());
        }
        else {
            texture.clear();
            if (!pixbuf.empty()) {
                texture = painter().create_texture(pixbuf);
            }
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
bool ImageView::timer_event(TimerEvent &event) {
    if (event.timerid() == timer) {
        int prev_delay = delay;

        if (image.empty()) {
            del_timer(timer);
            timer = 0;
            return true;
        }

        frame += 1;
        if (frame >= image.count_frame()) {
            // End of 
            // BTK_LOG("[ImageView::PlayImage] Reach max frame\n");
            frame = 0;
        }

        if (!image.read_frame(frame, pixbuf, &delay)) {
            // Error
            BTK_LOG("[ImageView::PlayImage] Failed to decode frame %d\n", frame);
            del_timer(timer);
            timer = 0;
            return true;
        }
        dirty = true;
        if (delay != -1) {
            // We could not reuse timer
            if (prev_delay != delay) {
                del_timer(timer);
                timer = 0;
                timer = add_timer(delay);
            }
            // else {
            //     BTK_LOG("[ImageView::PlayImage] Reuse timer\n");
            // }
        }
        else {
            // No delay, stop
            del_timer(timer);
            timer = 0;
        }
        repaint();
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


// ProgressBar
ProgressBar::ProgressBar(Widget *parent) {
    if (parent) {
        parent->add_child(this);
    }
}
ProgressBar::~ProgressBar() {

}

void ProgressBar::set_range(int64_t min, int64_t max) {
    _min = min;
    _max = max;
    _range_changed.emit();
    repaint();
}
void ProgressBar::set_value(int64_t value) {
    value = clamp(value, _min, _max);
    _value = value;
    _value_changed.emit();
    repaint();
}
void ProgressBar::set_direction(Direction direction) {
    _direction = direction;
    repaint();
}
void ProgressBar::set_text_visible(bool visible) {
    _text_visible = visible;
    repaint();
}
void ProgressBar::reset() {
    set_value(_min);

    // TODO : reset animation
}
bool ProgressBar::paint_event(PaintEvent &) {
    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect.apply_margin(2.0f);

    gc.set_antialias(true);

    // Background
    gc.set_color(style->background);
    gc.fill_rect(border);

    // Progress
    gc.set_color(style->highlight);

    auto bar = border;

    switch(_direction) {
        case LeftToRight : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            break;
        }
        case RightToLeft : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            bar.x += border.w - bar.w;
            break;
        }
        case TopToBottom : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            break;
        }
        case BottomToTop : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            bar.y += border.h - bar.h;
            break;
        }
    }

    gc.fill_rect(bar);

    if (_text_visible) {
        int percent = (_value - _min) * 100 / (_max - _min);
        auto text = u8string::format("%d %%", percent);

        gc.set_text_align(Alignment::Center | Alignment::Middle);
        gc.set_font(font());

        gc.set_color(style->text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );

        gc.push_scissor(bar);
        gc.set_color(style->highlight_text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.pop_scissor();
    }


    // Border
    gc.set_color(style->border);
    gc.draw_rect(border);

    // End
    gc.set_antialias(true);
    
    return true;
}

BTK_NS_END