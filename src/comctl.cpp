#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

// Common useful widgets

BTK_NS_BEGIN

Button::Button(Widget *parent, u8string_view txt) : AbstractButton(parent) {
    auto style = this->style();
    resize(style->button_width, style->button_height);

    if (!txt.empty()) {
        set_text(txt);
    }
}
Button::~Button() {

}

bool Button::paint_event(PaintEvent &event) {
    BTK_UNUSED(event);

    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto &gc  = this->painter();

    // Begin draw
    auto border = rect.apply_margin(2.0f);

    // Disable antialiasing
    gc.set_antialias(false);

    Color c;

    // Background
    if (_pressed) {
        c = style->highlight;
    }
    else{
        c = style->background;
    }
    gc.set_color(c.r, c.g, c.b, c.a);
    gc.fill_rect(border);

    if (_entered && !_pressed) {
        c = style->highlight;
    }
    else{
        c = style->border;
    }

    // Border
    if (!(_flat && !_pressed && !_entered)) {
        // We didnot draw border on _flat mode
        gc.set_color(c.r, c.g, c.b, c.a);
        gc.draw_rect(border);
    }

    // Text
    if (!_text.empty()) {
        if (_pressed) {
            c = style->highlight_text;
        }
        else{
            c = style->text;
        }
        gc.set_text_align(Alignment::Center | Alignment::Middle);
        gc.set_font(font());
        gc.set_color(c.r, c.g, c.b, c.a);

        gc.push_scissor(border);
        gc.draw_text(
            _text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.pop_scissor();
    }

    // Recover antialiasing
    gc.set_antialias(true);
    
    return true;
}
bool Button::mouse_press(MouseEvent &event) {
    if (event.button() == MouseButton::Left) {
        _pressed = true;

        repaint();
    }
    return true;
}
bool Button::mouse_release(MouseEvent &event) {
    if (_pressed) {
        _clicked.emit();
        _pressed = false;
        
        repaint();
    }
    return true;
}
bool Button::mouse_enter(MotionEvent &event) {
    _entered = true;
    _howered.emit();
    repaint();
    return true;
}
bool Button::mouse_leave(MotionEvent &event) {
    _entered = false;
    _pressed = false;
    repaint();
    return true;
}

Size Button::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}

RadioButton::RadioButton(Widget *p) : AbstractButton(p) {
    resize(size_hint());
}
RadioButton::~RadioButton() {

}
bool RadioButton::paint_event(PaintEvent &) {
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect().apply_margin(style->margin);

    // Clac the center circle (Left, Middle)
    float cx = border.x + style->radio_button_circle_pad + style->radio_button_circle_r / 2;
    float cy = border.y + border.h / 2;

    gc.set_antialias(true);

    // Draw background
    gc.set_color(style->background);
    gc.fill_circle(cx, cy, style->radio_button_circle_r);
    if (_checked) {
        // Draw inner box
        gc.set_color(style->highlight);
        gc.fill_circle(cx, cy, style->radio_button_circle_r);
        gc.set_color(style->background);
        gc.fill_circle(cx, cy, style->radio_button_circle_r - style->radio_button_circle_inner_pad);
    }
    else {
        // We didnot draw border at checked
        if (_entered) {
            gc.set_color(style->highlight);
        }
        else {
            gc.set_color(style->border);
        }
        gc.draw_circle(cx, cy, style->radio_button_circle_r);
    }

    // Draw text if exist
    if (!_text.empty()) {
        gc.push_scissor(border);
        gc.set_font(font());
        gc.set_text_align(Alignment::Left | Alignment::Middle);
        gc.set_color(style->text);
        gc.draw_text(
            _text,
            cx + style->radio_button_circle_r + style->radio_button_text_pad,
            cy
        );

        gc.pop_scissor();
    }

    return true;
}
bool RadioButton::mouse_press(MouseEvent &event) {
    _pressed = true;

    repaint();
    return true;
}
bool RadioButton::mouse_release(MouseEvent &event) {
    _pressed = false;
    if (!_checked) {
        _checked = true;
        _clicked.emit();
    }
    repaint();
    return true;
}
bool RadioButton::mouse_enter(MotionEvent &event) {
    _entered = true;
    _howered.emit();
    repaint();
    return true;
}
bool RadioButton::mouse_leave(MotionEvent &event) {
    _entered = false;
    _pressed = false;
    repaint();
    return true;
}
Size RadioButton::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}

// TextEdit
TextEdit::TextEdit(Widget *parent,u8string_view s) : Widget(parent) {
    set_focus_policy(FocusPolicy::Mouse);
    _text = s;
    _margin = 2.0f;
    // auto style = this->style();
    // resize(style->textedit_width, style->textedit_height);
    _text_changed.connect([this]() {
        _lay.set_font(font());
        _lay.set_text(_text);
    });
}
TextEdit::~TextEdit() {}

bool TextEdit::focus_gained(FocusEvent &) {
    start_textinput();
    set_textinput_rect(rect());

    _has_focus = true;
    _show_cursor = true;
    repaint();

    _timerid = add_timer(500);
    return true;
}
bool TextEdit::focus_lost(FocusEvent &) {
    _has_focus = false;
    _show_cursor = false;
    clear_sel();

    del_timer(_timerid);
    _timerid = 0;

    repaint();
    return true;
}
bool TextEdit::timer_event(TimerEvent &event) {
    if (event.timerid() == _timerid) {
        _show_cursor = !_show_cursor;
        if (!has_selection()) {
            repaint();
        }
    }
    return true;
}
bool TextEdit::mouse_enter(MotionEvent &) {
    _entered = true;
    repaint();
    return true;
}
bool TextEdit::mouse_leave(MotionEvent &) {
    _entered = false;
    repaint();
    return true;
}
bool TextEdit::mouse_press(MouseEvent &event) {
    if (_text.empty() || event.button() != MouseButton::Left) {
        return true;
    }

    size_t pos = get_pos_from(event.position());
    BTK_LOG("LineEdit::handle_mouse %d\n",int(pos));

    _cursor_pos = pos;
    // Reset empty selection
    clear_sel();

    repaint();

    return true;
}
bool TextEdit::key_press(KeyEvent &event) {
    switch(event.key()){
        case Key::Backspace: {
            if (has_selection()) {
                //Delete selection
                auto [start,end] = sel_range();

                do_delete(start,end);
                _cursor_pos = start;

                clear_sel();
                repaint();
                // Notify
                _text_changed.emit();
            }
            else if(! _text.empty() && _cursor_pos > 0) {
                // Normal delete
                do_delete(_cursor_pos - 1, _cursor_pos);
                if(_cursor_pos != 0){
                    _cursor_pos --;
                }
                repaint();
                // Notify
                _text_changed.emit();
            }
            break;
        }
        case Key::Right: {
            if(_cursor_pos < _text.length()){
                _cursor_pos ++;
            }
            clear_sel();
            repaint();
            break;
        }
        case Key::Left: {
            if(_cursor_pos > 0){
                _cursor_pos --;
            }
            clear_sel();
            repaint();
            break;
        }
        case Key::V: {
            // Ctrl + V
            if (int(event.modifiers() & Modifier::Ctrl)) {
                BTK_LOG("Ctrl + V\n");
                auto text = driver()->clipboard_get();
                if (!text.empty()) {
                    do_paste(text);
                }
            } 
            break;
        }
        case Key::C: {
            //Ctrl + C
            if (int(event.modifiers() & Modifier::Ctrl)) {
                BTK_LOG("Ctrl + C\n");
                if (has_selection()) {
                    driver()->clipboard_set(selection_text().c_str());
                }
                else{
                    driver()->clipboard_set(_text.c_str());
                }
            }
            break;
        }
        case Key::X: {
            if (int(event.modifiers() & Modifier::Ctrl) && has_selection()) {
                BTK_LOG("Ctrl + X\n");
                // Set to clipbiard
                driver()->clipboard_set(selection_text().c_str());
                // Remove it
                auto [start,end] = sel_range();
                do_delete(start,end);
                _cursor_pos = start;

                clear_sel();
                repaint();
                // Notify
                _text_changed.emit();
            }
            break;
        }
        case Key::Return: {
            _enter_pressed.emit();
            [[fallthrough]];
        }
        default : {
            // Forwards to parent
            return false;
        }
    }
    return true;
}
bool TextEdit::textinput_event(TextInputEvent &event) {
    if(event.text().size() == 0){
        return true;
    }
    do_paste(event.text());
    return true;
}
bool TextEdit::drag_begin(DragEvent &) {
    // Set Selection to current cursor
    _has_sel = true;
    _sel_begin = _cursor_pos;
    _sel_end   = _cursor_pos;

    repaint();
    return true;
}
bool TextEdit::drag_end(DragEvent &) {
    // No selection
    if (_sel_begin == _sel_end) {
        BTK_LOG("TextEdit::drag_end no selection\n");
        clear_sel();
    }

    repaint();
    return true;
}
bool TextEdit::drag_motion(DragEvent &event) {
    _cursor_pos = get_pos_from(event.position());
    _sel_begin = _cursor_pos;
    repaint();

    return true;
}
bool TextEdit::change_event(Event &event) {
    if (event.type() == Event::FontChanged) {
        _lay.set_font(font());
    }
    return true;
}
void TextEdit::do_paste(u8string_view txt) {
    if (has_selection()) {
        auto [start,end] = sel_range();

        //Has selection, notify before delete
        // if(not _signal_value_deleted.empty()){
        //     _signal_value_deleted(cur_text.substr(start,end - start));
        // }
        
        // BTK_LOGINFO("LineEdit::delete "BTK_CYELLOW("[%d,%d)"),int(start),int(end));

        _text.replace(
            start,
            end - start,
            txt
        );

        //Make the cur to the pasted end
        _cursor_pos = start + txt.length();

        clear_sel();
    }
    else{
        _text.insert(_cursor_pos, txt);
        _cursor_pos += txt.length();
    }

    repaint();

    _text_changed.emit();
}
void TextEdit::do_delete(size_t start, size_t end) {
    _text.erase(start,end - start);
    _text_changed.emit();

    repaint();
}
void TextEdit::clear_sel() {
    _has_sel = false;
    _sel_begin = 0;
    _sel_end = 0;
}
void TextEdit::set_text(u8string_view txt) {
    _text = txt;
    _cursor_pos = 0;
    repaint();
    _text_changed.emit();
}
void TextEdit::set_placeholder(u8string_view txt) {
    _placeholder = txt;
    repaint();
}
bool TextEdit::paint_event(PaintEvent &) {
    auto &p = painter();
    auto style = this->style();

    auto border = rect().cast<float>().apply_margin(2.0f);

    p.set_antialias(false);
    // Border and background
    p.set_color(style->background);
    p.fill_rect(border);
    if (_has_focus || _entered) {
        p.set_color(style->highlight);
    }
    else {
        p.set_color(style->border);
    }
    p.draw_rect(border);

    //Draw txt

    // p.use_font(font());
    p.set_font(font());
    p.set_text_align(_align);
    // Draw text
    auto txt_rect = border.apply_margin(_margin);

    _txt_pos.x = txt_rect.x;
    _txt_pos.y = txt_rect.y + txt_rect.h / 2;

    // Decrease 1.0f to make sure the cursor has properly space to draw
    p.push_scissor(txt_rect.apply_margin(-1.0f));
    if (!_text.empty()) {
        p.set_color(style->text);
        p.draw_text(_lay, _txt_pos.x, _txt_pos.y);
        // Use TextLayout directly
    }
    else if(!_placeholder.empty()) {
        //Draw place holder
        //Gray color
        p.set_color(Color::Gray);
        p.draw_text(_placeholder, _txt_pos.x, _txt_pos.y);
    }

    if (has_selection()) {
        // Draw selection
        auto [w, h] = _lay.size();
        auto [start,end] = sel_range();
        TextHitResults result;
        FRect sel_rect;
        // Get selection box begin
        _lay.hit_test_range(0, start, _txt_pos.x, _txt_pos.y - h / 2, &result);

        sel_rect.x = result.back().box.x + result.back().box.w;
        sel_rect.y = result.back().box.y;

        // Get selection box end
        _lay.hit_test_range(0, end, _txt_pos.x, _txt_pos.y - h / 2, &result);

        sel_rect.w = result.back().box.x + result.back().box.w - sel_rect.x;
        sel_rect.h = result.back().box.h;

        // Draw highlight box
        p.set_color(style->highlight);
        p.fill_rect(sel_rect);

        // Draw text again to cover selection
        p.push_scissor(sel_rect);
        p.set_color(style->highlight_text);
        p.draw_text(_lay, _txt_pos.x, _txt_pos.y);
        p.pop_scissor();
    }
    else if (_show_cursor) {
        // Draw cursor
        if (_cursor_pos == 0 && _text.empty()) {
            float h = font().size();
            float y = _txt_pos.y - h / 2;
            p.set_color(style->text);
            p.draw_line(
                _txt_pos.x, y,
                _txt_pos.x, y + h
            );
            // printf("%f\n", h);
        }
        else{
            float cursor_x;
            float cursor_y;
            float cursor_h;
            TextHitResults result;
            auto [w, h] = _lay.size();
            
            // Get cursor position by it
            if (_lay.hit_test_range(0, _cursor_pos, _txt_pos.x, _txt_pos.y - h / 2, &result)) {
                auto box = result.back().box;

                cursor_x = box.x + box.w;
                cursor_y = box.y;
                cursor_h = box.h;
                // Draw cursor
                p.set_color(style->text);
                p.draw_line(
                    cursor_x, cursor_y,
                    cursor_x, cursor_y + cursor_h
                );
            }
        }
    }

    p.pop_scissor();
    return true;
}
bool TextEdit::has_selection() const {
    return _has_sel && (_sel_begin != _sel_end);
}
size_t TextEdit::get_pos_from(const Point &p) {
    TextHitResult result;
    // Get test margin
    auto trect = rect().cast<float>().apply_margin(2.0f);
    trect = trect.apply_margin(_margin);

    auto [w, h] = _lay.size();

    FPoint start = {trect.x, trect.y + trect.h / 2 - h / 2};
    auto point = p.cast<float>();

    point.x -= start.x;
    point.y -= start.y;

    // BTK_LOG("Transformed point %f, %f\n", point.x, point.y);

    if (_lay.hit_test(point.x, point.y, &result)) {
        size_t pos = result.text;
        if (result.trailing) {
            pos += 1;
        }
        return pos;
    }
    return 0;
}


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
        return Size(pixbuf.width(), pixbuf.height());
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

    gc.set_antialias(false);

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

// Slider
// TODO : This widget is hardcoded, so we have to think how to make it more flexible.
Slider::Slider(Widget *p) : Widget(p) {
    resize(size_hint());
}
Slider::~Slider() {

}
Size Slider::size_hint() const {
    auto s = style();
    if (_orientation == Horizontal) {
        return Size(100, s->slider_height);
    }
    else {
        return Size(s->slider_width, 100);
    }
}
void Slider::set_value(int64_t value) {
    value = clamp(value, _min, _max);
    _value = value;
    _value_changed.emit();
    repaint();
}
void Slider::set_range(int64_t min, int64_t max) {
    _min = min;
    _max = max;
    _value = clamp(_value, _min, _max);
    _range_changed.emit();
    _value_changed.emit();
    repaint();
}
bool Slider::paint_event(PaintEvent &) {
    auto content = content_rect();
    auto bar     = bar_rect();
    auto s       = style();
    auto &p = painter();

    p.set_antialias(false);

    // Fill the content rect
    p.set_color(s->background);
    p.fill_rect(content);
    p.set_color(s->border);
    p.draw_rect(content);

    // Fill the content rect by the value
    p.set_color(s->highlight);
    if (_orientation == Horizontal) {
        content.w = (content.w * _value) / (_max - _min);
    }
    else {
        content.h = (content.h * _value) / (_max - _min);
    }
    p.fill_rect(content);

    // Draw the bar
    if (_pressed) {
        p.set_color(s->highlight);
    }
    else {
        p.set_color(s->background);
    }
    p.fill_rect(bar);

    if (_hovered) {
        p.set_color(s->highlight);
    }
    else {
        p.set_color(s->border);
    }
    p.draw_rect(bar);

    return true;
}
bool Slider::mouse_press(MouseEvent &event) {
    if (event.button() == MouseButton::Left) {
        _pressed = true;

        // Calc the value
        auto rect = content_rect();
        double v;

        if (_orientation == Horizontal) {
            v = (event.x() - rect.x) * (_max - _min) / rect.w;
        }
        else {
            v = (event.y() - rect.y) * (_max - _min) / rect.h;
        }

        set_value(std::round(v));
    }
    return true;
}
bool Slider::mouse_release(MouseEvent &event) {
    _pressed = false;
    repaint();
    return true;
}
bool Slider::mouse_enter(MotionEvent &event) {
    _hovered = true;
    repaint();
    return true;
}
bool Slider::mouse_leave(MotionEvent &event) {
    if (!_dragging) {
        // Is dragging, we don't reset the pressed state
        _pressed = false;
    }
    _hovered = false;
    repaint();
    return true;
}
bool Slider::mouse_wheel(WheelEvent &event) {
    set_value(_value + event.y());
    return true;
}

bool Slider::drag_begin(DragEvent &event) {
    _slider_pressed.emit();
    if (_pressed) {
        _dragging = true;
        repaint();
        return true;
    }
    return false;
}
bool Slider::drag_motion(DragEvent &event) {
    // Update the value by it
    
    auto   rect = content_rect();
    double v;
    if (_orientation == Horizontal) {
        v = (event.x() - rect.x) * (_max - _min) / rect.w;
    }
    else {
        v = (event.y() - rect.y) * (_max - _min) / rect.h;
    }
    set_value(std::round(v));

    return true;
}
bool Slider::drag_end(DragEvent &event) {
    _slider_released.emit();
    _dragging = false;
    _pressed  = false;
    repaint();
    return true;
}

FRect Slider::content_rect() const {
    auto r = rect().apply_margin(style()->margin);

    FPoint center;
    center.x = r.x + r.w / 2;
    center.y = r.y + r.h / 2;

    // Get the size of the bar
    float w = 5;
    float h = 5;

    if (_orientation == Horizontal) {
        r.y = center.y - h / 2;
        r.h = h;
    }
    else {
        r.x = center.x - w / 2;
        r.w = w;
    }
    return r;
}
FRect Slider::bar_rect() const {
    // Calc the bar rectangle
    FRect prev = content_rect();
    FRect r = prev;
    if (_orientation == Horizontal) {
        r.w = 7;
        r.h = 18;
        r.x = r.x + (prev.w * _value) / (_max - _min);
        r.y = r.y - r.h / 2 + prev.h / 2; //< Center the bar
    }
    else {
        r.h = 7;
        r.w = 18;
        r.y = r.y + (prev.h * _value) / (_max - _min);
        r.x = r.x - r.w / 2 + prev.w / 2; //< Center the bar
    }
    return r;
}

// Layout
Layout::Layout(Widget *p) {
    if (p) {
        attach(p);
    }
}
Layout::~Layout() {
    if (_widget) {
        attach(nullptr);
    }
}
void Layout::attach(Widget *w) {
    auto hook = [](Object *, Event &event, void *self) -> bool {
        static_cast<Layout *>(self)->run_hook(event);
        return false; //< nodiscard
    };
    if (_hooked) {
        // Detach the hook from the previous widget
        _widget->del_event_filter(hook, this);
    }
    _widget = w;

    if (_widget != nullptr) {
        _widget->add_event_filter(hook, this);
        _hooked = true;
    }
}
void Layout::set_margin(Margin ma) {
    _margin = ma;
    mark_dirty();
}
void Layout::set_spacing(int spacing) {
    _spacing = spacing;
    mark_dirty();
}
void Layout::set_parent(Layout *l) {
    _parent = l;
    mark_dirty();
}
Layout *Layout::layout() {
    return this;
}
Widget *Layout::widget() const {
    return _widget;
}
// BoxLayout
BoxLayout::BoxLayout(Direction d) {
    _direction = d;
}
BoxLayout::~BoxLayout() {
    for (auto v : items) {
        delete v.first;
    }
}
void BoxLayout::add_item(LayoutItem *item) {
    mark_dirty();
    items.push_back(std::make_pair(item, ItemExtra()));
}
void BoxLayout::add_widget(Widget *w, int stretch, Alignment align) {
    if (w) {
        auto p = parent();
        if (p) {
            // To top
            while(p->parent()) {
                p = p->parent();
            }
        }
        else {
            p = this;
        }
        auto item = new WidgetItem(w);
        item->set_alignment(align);

        mark_dirty();
        items.push_back(std::make_pair(item, ItemExtra{stretch}));

        p->widget()->add_child(w);
    }
}
void BoxLayout::add_layout(Layout *l, int stretch) {
    if (l) {
        mark_dirty();
        items.push_back(std::make_pair(static_cast<LayoutItem*>(l), ItemExtra{stretch}));
        l->set_parent(this);
    }
}
void BoxLayout::add_stretch(int stretch) {
    mark_dirty();
    items.push_back(std::make_pair(static_cast<LayoutItem*>(new SpacerItem(0, 0)), ItemExtra{stretch}));
}
void BoxLayout::add_spacing(int spacing) {
    mark_dirty();
    Size size = {0, 0};
    if (_direction == RightToLeft || _direction == LeftToRight) {
        size.w = spacing;
    }
    else {
        size.h = spacing;
    }
    add_item(new SpacerItem(size.w, size.h));
    n_spacing_item += 1;
}
void BoxLayout::set_direction(Direction d) {
    _direction = d;
    mark_dirty();

    if (n_spacing_item) {
        // We need for and excahnge items w, h
        for (auto [item, extra] : items) {
            auto i = item->spacer_item();
            if (!i) {
                continue;
            }
            auto size = item->size_hint();
            std::swap(size.w, size.h);
            i->set_size(size.w, size.h);

            --n_spacing_item;
            if (!n_spacing_item) {
                break;
            }
        }
    }
}
int  BoxLayout::item_index(LayoutItem *item) {
    auto iter = items.begin();
    while (iter != items.end()) {
        if (iter->first == item) {
            break;
        }
        ++iter;
    }
    if (iter == items.end()) {
        return -1;
    }
    return iter - items.begin();
}
int  BoxLayout::count_items() {
    return items.size();
}
LayoutItem *BoxLayout::item_at(int idx) {
    if (idx < 0 || idx > items.size()) {
        return nullptr;
    }
    return items[idx].first;
}
LayoutItem *BoxLayout::take_item(int idx) {
    if (idx < 0 || idx > items.size()) {
        return nullptr;
    }
    auto it = items[idx];
    items.erase(items.begin() + idx);
    mark_dirty();

    if (it.first->spacer_item()) {
        n_spacing_item -= 1;
    }

    return it.first;
}
void BoxLayout::mark_dirty() {
    size_dirty = true;
    _dirty = true;
}
void BoxLayout::run_hook(Event &event) {
    switch (event.type()) {
        case Event::Resized : {
            _rect.x = 0;
            _rect.y = 0;
            _rect.w = event.as<ResizeEvent>().width();
            _rect.h = event.as<ResizeEvent>().height();
            if (!_except_resize) {
                mark_dirty();
                run_layout(nullptr);
            }
            break;
        }
        case Event::Show : {
            run_layout(nullptr);
            break;
        }
    }
}
void BoxLayout::run_layout(const Rect *dst) {
    if (items.empty() || !_dirty) {
        return;
    }
    // Calc need size
    Rect r;

    if (!dst) {
        // In tree top, measure it
        Size size;
        size = size_hint();
        r    = rect();
        if (r.w < size.w || r.h < size.h) {
            // We need bigger size
            BTK_LOG("BoxLayout resize to bigger size (%d, %d\n)", size.w, size.h);
            BTK_LOG("Current size (%d, %d)\n", r.w, r.h);

            assert(widget());
            
            _except_resize = true;
            widget()->set_minimum_size(size);
            widget()->resize(size);
            _except_resize = false;

            r.w = size.w;
            r.h = size.h;
        }
    }
    else {
        // Sub, just pack
        r = *dst;
    }
    r = r.apply_margin(margin());

    BTK_LOG("BoxLayout: run_layout\n");

    // Begin
    auto iter = items.begin();
    auto move_next = [&, this]() -> bool {
        if (_direction == RightToLeft || _direction == BottomToTop) {
            // Reverse
            if (iter == items.begin()) {
                // EOF
                return false;
            }
            --iter;
            return true;
        }
        ++iter;
        return iter != items.end();
    };
    auto move_to_begin = [&, this]() -> void {
        if (_direction == RightToLeft || _direction == BottomToTop) {
            iter = --items.end();
        }
        else {
            iter = items.begin();
        }
    };
    auto is_vertical = (_direction == RightToLeft || _direction == LeftToRight);

    move_to_begin();
    int stretch_sum = 0; //< All widget stretch
    int space = 0;

    if (is_vertical) {
        space = r.w - spacing() * (items.size() - 1);
    }
    else {
        space = r.h - spacing() * (items.size() - 1);
    }
    // Useable space
    do {
        auto &extra = iter->second;
        auto  item  = iter->first;
        auto  hint  = item->size_hint();

        // Save current alloc size
        extra.alloc_size = hint;

        if (is_vertical) {
            space -= hint.w;
        }
        else {
            space -= hint.h;
        }

        if (extra.stretch) {
            // Has stretch
            stretch_sum += extra.stretch;
        }
    }
    while(move_next());

    // Process extra space if
    if (space) {
        move_to_begin();
        BTK_LOG("BoxLayout: has extra space\n");
        if (stretch_sum) {
            // Alloc space for item has stretch
            int part = space / stretch_sum;
            do {
                auto &extra = iter->second;
                auto  item = iter->first;

                if (!extra.stretch) {
                    continue;
                }
                if (is_vertical) {
                    extra.alloc_size.w += part * extra.stretch;
                }
                else {
                    extra.alloc_size.h += part * extra.stretch;
                }
            }
            while(move_next());
        }
        else {
            // Alloc space for item has stretch 0 by size policy
            // TODO : temp use avg
            int part = space / items.size();
            do {
                auto &extra = iter->second;
                auto  item = iter->first;

                if (is_vertical) {
                    extra.alloc_size.w += part;
                }
                else {
                    extra.alloc_size.h += part;
                }
            }
            while(move_next());
        }
    }


    // Assign
    move_to_begin();
    int x = r.x;
    int y = r.y;
    do {
        auto &extra = iter->second;
        auto  item  = iter->first;
        auto  size  = extra.alloc_size;
        auto  rect  = Rect(x, y, size.w, size.h);
        // Make it to box
        if (is_vertical) {
            rect.h = r.h;
        }
        else {
            rect.w = r.w;
        }

        if (item->alignment() != Alignment{}) {
            // Align it
            auto perfered = item->size_hint();
            auto alig     = item->alignment();

            if (bool(alig & Alignment::Left)) {
                rect.w = perfered.w;
            }
            else if (bool(alig & Alignment::Right)) {
                rect.x = rect.x + rect.w - perfered.w;
                rect.w = perfered.w;
            }
            else if (bool(alig & Alignment::Center)) {
                rect.x = rect.x + rect.w / 2 - perfered.w / 2;
                rect.w = perfered.w;
            }

            if (bool(alig & Alignment::Top)) {
                rect.h = perfered.h;
            }
            else if (bool(alig & Alignment::Bottom)) {
                rect.y = rect.y + rect.h - perfered.h;
                rect.h = perfered.h;
            }
            else if (bool(alig & Alignment::Middle)) {
                rect.y = rect.y + rect.h / 2 - perfered.h / 2;
                rect.h = perfered.h;
            }
        }

        item->set_rect(rect);

        if (is_vertical) {
            x += size.w;
            x += spacing();
        }
        else {
            y += size.h;
            y += spacing();
        }
    }
    while(move_next());

    _dirty = false;
}
void BoxLayout::set_rect(const Rect &r) {
    auto w = widget();
    if (w != nullptr) {
        _except_resize = true;
        w->set_rect(r.x, r.y, r.w, r.h);
        _except_resize = false;
    }
    _rect = r;
    mark_dirty();
    run_layout(&_rect);
}
Size BoxLayout::size_hint() const {
    if (size_dirty) {
        // Measure the width
        auto spacing = this->spacing();
        auto margin = this->margin();
        Rect result = {0, 0, 0, 0};

        switch (_direction) {
            case LeftToRight : {
                [[fallthrough]];
            }
            case RightToLeft : {
                for (auto [item, extra] : items) {
                    auto hint = item->size_hint();
                    
                    result.w += hint.w;
                    result.h = max(result.h, hint.h);
                }

                if (items.size() > 0) {
                    // Add spacing
                    result.w += spacing * (items.size() - 1);
                }
                break;
            }
            case BottomToTop : {
                [[fallthrough]];
            }
            case TopToBottom : {
                for (auto [item, extra] : items) {
                    auto hint = item->size_hint();
                    
                    result.h += hint.h;
                    result.w = max(result.w, hint.w);
                }

                if (items.size() > 0) {
                    // Add spacing
                    result.h += spacing * (items.size() - 1);
                }
                break;
            }
        }
        cached_size = result.unapply_margin(margin).size();
        size_dirty = false;
    }
    BTK_LOG("BoxLayout size_hint (%d, %d)\n", cached_size.w, cached_size.h);
    return cached_size;
}
Rect BoxLayout::rect() const {
    // auto w = widget();
    // if (w) {
    //     return w->rect();
    // }
    return _rect;
}


BTK_NS_END