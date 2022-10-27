#include "build.hpp"

#include <Btk/widgets/textedit.hpp>
#include <Btk/context.hpp>
#include <Btk/event.hpp>


BTK_NS_BEGIN

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
    if (!s.empty()) {
        _lay.set_font(font());
        _lay.set_text(_text);
    }
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
    repaint();
    return true;
}
bool TextEdit::mouse_leave(MotionEvent &) {
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
Size TextEdit::size_hint() const {
    auto s = style();
    return Size(s->button_width, s->button_height); //< Temp as Button
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

    p.set_antialias(true);
    // Border and background
    p.set_color(style->background);
    p.fill_rect(border);
    if (_has_focus || under_mouse()) {
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


BTK_NS_END