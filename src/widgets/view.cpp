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
    repaint();
}
void Label::set_text_align(Alignment alig) {
    _align = alig;
    repaint();
}
bool Label::paint_event(PaintEvent &) {
    auto border = FRect(0, 0, size());
    auto &painter = this->painter();

    // Get textbox
    auto size = _layout.size();
    auto textbox = border.align_object(size, _align);

    painter.set_font(font());
    painter.set_text_align(AlignLeft + AlignTop);
    painter.set_brush(palette().text());

    painter.push_scissor(border);
    painter.draw_text(
        _layout,
        textbox.x,
        textbox.y
    );
    painter.pop_scissor();

    return true;
}
bool Label::change_event(ChangeEvent &event) {
    if (event.type() == Event::FontChanged) {
        _layout.set_font(font());
        repaint();
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
            dst.x = (size.w - dst.w) / 2;
            dst.y = (size.h - dst.h) / 2;
        }
        else{
            dst = FRect(0, 0, size());
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
    auto rect = FRect(0, 0, size());
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect.apply_margin(2.0f);

    // In low dpi screens, we didnot need to antialiasing the rectangle
    gc.set_antialias(window_dpi().x > 96.0f);

    // Background
    gc.set_brush(palette().input());
    gc.fill_rect(border);

    // Progress
    gc.set_brush(palette().hightlight());

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

        gc.set_brush(palette().text());
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );

        gc.push_scissor(bar);
        gc.set_brush(palette().hightlighted_text());
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.pop_scissor();
    }


    // Border
    gc.set_brush(palette().border());
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
    update_item(&*iter);
}
void ListBox::set_flat(bool value) {
    _flat = value;
    repaint();
}
void ListBox::add_item(const ListItem &item) {
    auto &ref = _items.emplace_back(item);
    // Update internal data
    update_item(&ref);
}
bool ListBox::paint_event(PaintEvent &event) {
    auto &p = painter();
    auto r  = FRect(0, 0, size()).apply_margin(style()->margin);

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

    if (!_flat) {
        p.draw_rect(r);
    }

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
        // Clac the box width, height
        auto layout_size = item.layout.size();
        float box_w = layout_size.w;
        float box_h = layout_size.h;
        float text_x = x;
        float text_y = y;

        // Check if has image, calc the img box
        if (!item.image.empty()) {
            box_w +=     item.image_size.w + style()->margin * 2;
            box_h  = max(item.image_size.h + style()->margin * 2, box_h);

            text_x += (item.image_size.w + style()->margin * 2);
        }
        else {
            // Add margin
            text_x += style()->margin;
        }

        // _xtranslate < 0
        // - it to make box fit
        FRect client(x, y, r.w - _xtranslate, box_h);
        if (r.is_intersected(client)) {
            // Draw 
            if (_current == idx) {
                // Draw background
                p.set_brush(palette().hightlight());
                p.set_alpha(0.4f);
                p.fill_rect(client);
                p.set_alpha(1.0f);

                p.set_brush(palette().text());
                // p.set_brush(palette().hightlighted_text());
            }
            else if (_hovered == idx) {
                // Is Hovered
                p.set_brush(palette().hightlight());
                p.set_alpha(0.2f);
                p.fill_rect(client);
                p.set_alpha(1.0f);
                
                p.set_brush(palette().text());
            }
            else {
                p.set_brush(palette().text());
            }
            p.draw_text(item.layout, text_x, text_y);

            // Draw Image
            if (!item.image.empty()) {
                if (item.image_brush.type() != BrushType::Bitmap) {
                    item.image_brush.set_image(item.image);
                }
                p.set_brush(item.image_brush);
                p.fill_rect(
                    x + style()->margin, 
                    y + style()->margin, 
                    item.image_size.w, 
                    item.image_size.h
                );
            }

            painted = true;
        }
        else if (painted) {
            break;
        }
        // Increase
        idx += 1;
        y   += box_h;
        y   += _spacing;
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
    if (i && event.clicks() == 1) {
        // Single click
        _item_clicked.emit(i);
    }
    if (i && event.clicks() % 2 == 0) {
        BTK_LOG("Item double clicked\n");
        _item_double_clicked.emit(i);
    }
    return true;
}
bool ListBox::mouse_wheel(WheelEvent &event) {
    if (_vslider->visible()) {
        // ENABLED 
        bool v = _vslider->handle(event);
        set_mouse_hover(mouse_position());
        return v;
    }
    else {
        // Items is not encough
        BTK_LOG("[ListBox] more item required\n")
        signal_more_item_required().emit();
    }
    return false;
}
bool ListBox::mouse_motion(MotionEvent &event) {
    // TODO : Handle Mosue motion to tigger _item_enter
    // Maybe we need cache here
    set_mouse_hover(event.position());
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
    auto item_viewport = FRect(0, 0, size()).apply_margin(s->margin).apply_margin(s->margin);

    float cur_x = item_viewport.x;
    float cur_y = item_viewport.y;
    float box_w = 0;
    float box_h = 0;
    for (auto &item : _items) {
        auto [w, h] = item.layout.size();
        box_w = w;
        box_h = h;
        if (!item.image.empty()) {
            box_h  = max(box_h, item.image_size.h + style()->margin * 2);
            box_w += item.image_size.w + style()->margin * 2;
        }

        FRect rect(cur_x, cur_y, (item_viewport.w - _xtranslate), box_h);
        if (rect.contains(x, y)) {
            return &item;
        }

        cur_y   += box_h;
        cur_y   += _spacing;
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
void ListBox::update_item(ListItem *item) {
    if (!item) {
        return;
    }
    if (item->font.empty()) {
        item->font = font();
    }
    if (!item->image_size.is_valid() && !item->image.empty()) {
        // Invalid, does not sepcial it
        item->image_size = Size(
            style()->icon_width,
            style()->icon_height
        );
    }

    item->layout.set_text(item->text);
    item->layout.set_font(item->font);

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
void ListBox::set_current_index(int idx) {
    if (idx < 0) {
        idx = -1;
    }
    else if (idx >= _items.size()) {
        idx = -1;
    }
    _current = idx;
    repaint();
    _current_item_changed.emit();
}
void ListBox::set_mouse_hover(Point where) {
    auto item = item_at(where.x, where.y);
    auto new_hovered = index_of(item);
    if (new_hovered != _hovered) {
        _hovered = new_hovered;
        repaint();

        if (item) {
            _item_enter.emit(item);
        }
    }
}

int  ListBox::index_of(const ListItem *item) const {
    if (!item) {
        return -1;
    }
    return item - _items.data();
}
int  ListBox::count_items() const {
    return _items.size();
}

void ListBox::items_changed() {
    calc_slider();
    repaint();
}
void ListBox::calc_slider() {
    // Calc the Height of the string list
    auto s = style();
    auto item_viewport = FRect(0, 0, size()).apply_margin(s->margin).apply_margin(s->margin);

    auto [width, height] = calc_items_size();
    BTK_LOG("ListBox::calc_slider: width: %f, height: %f\n", width, height);

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

    // Check width is bigger than it
    if (width > item_viewport.w) {
        // Should enable slider
        float diff = width - item_viewport.w;
        auto cur = _hslider->value();

        _hslider->show();
        _hslider->set_page_step(item_viewport.w);
        _hslider->set_single_step(width / _items.size());
        _hslider->set_range(0, diff);
        _hslider->set_value(min<double>(diff, cur));

        // BTK_LOG("Slider line step %lf\n", slider->single_step());
        // BTK_LOG("Slider page step %lf\n", slider->page_step());
        
        // Place at
        _hslider->move(item_viewport.x, item_viewport.y + item_viewport.h - _hslider->height());
        _hslider->resize(item_viewport.w, _hslider->height());
    }
    else {
        _hslider->hide();
        _hslider->set_range(0, 100);
        _hslider->set_value(0);
        _xtranslate = 0;
    }
}
void ListBox::vslider_value_changed() {
    if (_vslider->visible()) {
        _ytranslate = -_vslider->value();
        repaint();

        // Check here
        if (_vslider->value() == _vslider->max()) {
            BTK_LOG("[ListBox] more item required\n")
            signal_more_item_required().emit();
        }
    }
}
void ListBox::hslider_value_changed() {
    if (_hslider->visible()) {
        _xtranslate = -_hslider->value();
        repaint();
    }
}
Size ListBox::size_hint() const {
    auto s = calc_items_size();
    if (s.w == 0 && s.h == 0) {
        return Size(0, 0);
    }
    // Add content margin, convert it to border
    s.w += style()->margin * 2;
    s.h += style()->margin * 2;
    return s;
}
FSize ListBox::calc_items_size() const {
    float height = 0.0f;
    float width = 0.0f;
    
    for (auto &item : _items) {
        // Calc the font
        auto [w, h] = item.layout.size();
        width   = max(width, w);

        // Calc the if has image
        if (!item.image.empty()) {
            // Margin Image Margin
            float box_width = item.image_size.w + style()->margin * 2 + w;
            float box_height = item.image_size.h + style()->margin * 2;

            width   = max(width, box_width);
            height += max(h, box_height);
        }
        else {
            height += h;            
        }

        height += _spacing;
    }

    return FSize(width, height);
}
BTK_NS_END