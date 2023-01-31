#include "build.hpp"

#include <Btk/widgets/slider.hpp>
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
bool Label::change_event(ChangeEvent &event) {
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

    // In low dpi screens, we didnot need to antialiasing the rectangle
    gc.set_antialias(window_dpi().x > 96.0f);

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

ListBox::ListBox(Widget *parent) : Widget(parent) {
    _vslider = new ScrollBar(this, Vertical);
    _vslider->hide();
    _vslider->signal_value_changed().connect(&ListBox::vslider_value_changed, this);

    _hslider = new ScrollBar(this, Horizontal);
    _hslider->hide();
    _hslider->signal_value_changed().connect(&ListBox::hslider_value_changed, this);

    set_focus_policy(FocusPolicy::Mouse);
}
ListBox::~ListBox() {

}

void ListBox::clear() {
    _items.clear();
    _current = -1;

    items_changed();
}
void ListBox::insert_item(size_t idx, const ListItem &item) {
    if (idx > _items.size()) {
        idx = _items.size();
    }
    auto iter = _items.insert(_items.begin() + idx, item);
    if (iter->font.empty()) {
        iter->font = font();
    }

    iter->layout.set_text(iter->text);
    iter->layout.set_font(iter->font);

    items_changed();
}
void ListBox::add_item(const ListItem &item) {
    auto &ref = _items.emplace_back(item);
    if (ref.font.empty()) {
        ref.font = font();
    }

    ref.layout.set_text(ref.text);
    ref.layout.set_font(ref.font);

    items_changed();
}
bool ListBox::paint_event(PaintEvent &event) {
    auto &p = painter();
    auto r  = rect().apply_margin(style()->margin);

    p.save();
    p.set_antialias(true);
    p.set_stroke_width(1.0f);

    // Draw border
    p.set_brush(palette().input());
    p.fill_rect(r);
    if (has_focus()) {
        p.set_brush(palette().hightlight());
    }
    else {
        p.set_brush(palette().border());
    }
    p.draw_rect(r);

    p.set_text_align(AlignLeft + AlignTop);

    // Get Text client area
    r = r.apply_margin(style()->margin);
    float x = r.x + _xtranslate;
    float y = r.y + _ytranslate;
    int idx = 0;

    p.save();
    p.scissor(r);

    bool painted = false; //< Has painted text    

    for (auto &item : _items) {
        auto [w, h] = item.layout.size();

        FRect client(x, y, r.w, h);
        if (r.is_intersected(client)) {
            // Draw 
            if (_current == idx) {
                // Draw background
                p.set_brush(palette().hightlight());
                p.fill_rect(client);

                p.set_brush(palette().hightlighted_text());
            }
            else {
                p.set_brush(palette().text());
            }
            p.draw_text(item.layout, x, y);

            painted = true;
        }
        else if (painted) {
            break;
        }
        // Increase
        idx += 1;
        y   += h;
    }
    p.restore();
    p.restore();
    return true;
}
bool ListBox::resize_event(ResizeEvent &event) {
    calc_slider();
    return true;
}
bool ListBox::mouse_press(MouseEvent &event) {
    auto i = item_at(event.x(), event.y());
    set_current_item(i);
    if (i) {
        _item_clicked.emit(i);
    }
    return true;
}
bool ListBox::mouse_wheel(WheelEvent &event) {
    if (_vslider->visible()) {
        // ENABLED 
        return _vslider->handle(event);
    }
    return false;
}
bool ListBox::mouse_motion(MotionEvent &event) {
    // TODO : Handle Mosue motion to tigger _item_enter
    // Maybe we need cache here
    return true;
}
bool ListBox::focus_gained(FocusEvent &event) {
    repaint();
    return true;
}
bool ListBox::focus_lost(FocusEvent &event) {
    repaint();
    return true;
}

ListItem *ListBox::item_at(float x, float y) {
    // Detranslate y
    y -= _ytranslate;
    x -= _xtranslate;

    auto s = style();
    auto item_viewport = rect().apply_margin(s->margin).apply_margin(s->margin);

    float cur_x = item_viewport.x;
    float cur_y = item_viewport.y;
    for (auto &item : _items) {
        auto [w, h] = item.layout.size();
        FRect rect(cur_x, cur_y, item_viewport.w, h);
        if (rect.contains(x, y)) {
            return &item;
        }

        cur_y   += h;
    }

    return nullptr;
}
ListItem *ListBox::item_at(int idx) {
    if (idx > _items.size() || idx < 0) {
        return nullptr;
    }
    return &_items[idx];
}
ListItem *ListBox::current_item() {
    if (_current < 0 || _current >= _items.size()) {
        return nullptr;
    }
    return &_items[_current];
}

void ListBox::remove_item(ListItem *item) {
    if (!item) {
        return;
    }
    auto idx = index_of(item);
    if (idx == _current) {
        // Current Index
        set_current_item(nullptr);
    }
    _items.erase(_items.begin() + idx);
    items_changed();
}

void ListBox::set_current_item(ListItem *item) {
    int now;
    if (!item) {
        now = -1;
    }
    else {
        now = (item - _items.data());
    }
    if (now == _current) {
        return;
    }
    _current = now;
    repaint();
    _current_item_changed.emit();
}

int  ListBox::index_of(const ListItem *item) const {
    if (!item) {
        return -1;
    }
    return item - _items.data();
}

void ListBox::items_changed() {
    calc_slider();
    repaint();
}
void ListBox::calc_slider() {
    // Calc the Height of the string list
    auto s = style();
    auto item_viewport = rect().apply_margin(s->margin).apply_margin(s->margin);
    float height = 0.0f;
    
    for (auto &item : _items) {
        auto [w, h] = item.layout.size();
        height += h;
    }


    // Check is bigger than slider
    if (height > item_viewport.h) {
        // Should enable slider
        float diff = height - item_viewport.h;
        auto cur = _vslider->value();

        _vslider->show();
        _vslider->set_page_step(item_viewport.h);
        _vslider->set_single_step(height / _items.size());
        _vslider->set_range(0, diff);
        _vslider->set_value(min<double>(diff, cur));

        // BTK_LOG("Slider line step %lf\n", slider->single_step());
        // BTK_LOG("Slider page step %lf\n", slider->page_step());
        
        // Place at
        _vslider->move(item_viewport.x + item_viewport.w - _vslider->width(), item_viewport.y);
        _vslider->resize(_vslider->width(), item_viewport.h);
    }
    else {
        // Disable
        _vslider->hide();
        _vslider->set_range(0, 100);
        _vslider->set_value(0);
        _ytranslate = 0;
    }
}
void ListBox::vslider_value_changed() {
    if (_vslider->visible()) {
        _ytranslate = -_vslider->value();
        repaint();
    }
}
void ListBox::hslider_value_changed() {
    if (_hslider->visible()) {
        _xtranslate = -_hslider->value();
        repaint();
    }
}
BTK_NS_END