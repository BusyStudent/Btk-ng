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
    _offset = {0.0f, 0.0f};
    // auto style = this->style();
    // resize(style->textedit_width, style->textedit_height);
    _lay.set_font(font());
    if (!s.empty()) {
        _lay.set_text(_text);
        _text_len = _text.length();
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

    move_cursor(pos);
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
                move_cursor(start);

                clear_sel();
                repaint();
            }
            else if(! _text.empty() && _cursor_pos > 0) {
                // Normal delete
                do_delete(_cursor_pos - 1, _cursor_pos);
                if(_cursor_pos != 0){
                    move_cursor(_cursor_pos - 1);
                }
                repaint();
            }
            break;
        }
        case Key::Right: {
            if(_cursor_pos < _text.length()){
                move_cursor(_cursor_pos + 1);
            }
            clear_sel();
            repaint();
            break;
        }
        case Key::Left: {
            if(_cursor_pos > 0){
                move_cursor(_cursor_pos - 1);
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
                move_cursor(start);

                clear_sel();
                repaint();
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
    move_cursor(get_pos_from(event.position()));
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
    if (!_multi) {
        if (txt.contains("\n")) {
            u8string us(txt);
            us.replace("\n", "");
            return do_paste(us);
        }
    }

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
        _lay.set_text(_text);
        _text_len = _text.length();

        //Make the cur to the pasted end
        move_cursor(start + txt.length());

        clear_sel();
    }
    else{
        _text.insert(_cursor_pos, txt);
        _lay.set_text(_text);
        _text_len = _text.length();
        move_cursor(_cursor_pos + txt.length());
    }

    repaint();

    _text_changed.emit();
}
void TextEdit::do_delete(size_t start, size_t end) {
    _text.erase(start,end - start);
    _lay.set_text(_text);
    _text_len = _text.length();
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
    _lay.set_text(_text);
    _text_len = _text.length();
    move_cursor(0);
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
    auto txt_rect = text_rectangle();
    auto txt_pos  = text_position();

    // Decrease 1.0f to make sure the cursor has properly space to draw
    p.push_scissor(txt_rect.apply_margin(-1.0f));
    if (!_text.empty()) {
        p.set_color(style->text);
        p.draw_text(_lay, txt_pos.x, txt_pos.y);
        // Use TextLayout directly
    }
    else if(!_placeholder.empty()) {
        //Draw place holder
        //Gray color
        p.set_color(Color::Gray);
        p.draw_text(_placeholder, txt_pos.x, txt_pos.y);
    }

    if (has_selection()) {
        // Draw selection
        auto [w, h] = _lay.size();
        auto [start,end] = sel_range();
        TextHitResults result;
        FRect sel_rect;
        // Get selection box begin
        _lay.hit_test_range(0, start, txt_pos.x, txt_pos.y - h / 2, &result);

        sel_rect.x = result.back().box.x + result.back().box.w;
        sel_rect.y = result.back().box.y;

        // Get selection box end
        _lay.hit_test_range(0, end, txt_pos.x, txt_pos.y - h / 2, &result);

        sel_rect.w = result.back().box.x + result.back().box.w - sel_rect.x;
        sel_rect.h = result.back().box.h;

        // Draw highlight box
        p.set_color(style->highlight);
        p.fill_rect(sel_rect);

        // Draw text again to cover selection
        p.push_scissor(sel_rect);
        p.set_color(style->highlight_text);
        p.draw_text(_lay, txt_pos.x, txt_pos.y);
        p.pop_scissor();
    }
    else if (_show_cursor) {
        // Draw cursor
        if (_cursor_pos == 0 && _text.empty()) {
            float h = font().size();
            float y = txt_pos.y - h / 2;
            p.set_color(style->text);
            p.draw_line(
                txt_pos.x, y,
                txt_pos.x, y + h
            );
            // printf("%f\n", h);
        }
        else {
            float cursor_x;
            float cursor_y;
            float cursor_h;
            TextHitResults result;
            auto [w, h] = _lay.size();
            
            // Get cursor position by it
            if (_lay.hit_test_range(0, _cursor_pos, txt_pos.x, txt_pos.y - h / 2, &result)) {
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
    auto [w, h] = _lay.size();

    FPoint start = text_position();
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
FPoint TextEdit::text_position() const {
    auto r = text_rectangle();
    FPoint ret;
    if (_multi) {
        ret = {r.x, r.y};
    }
    else {
        ret = {
            r.x,
            r.y + r.h / 2
        };
    }
    return ret + _offset;
}
FRect  TextEdit::text_rectangle() const {
    // Widget bounds => Text Rectangle
    return rect().apply_margin(style()->margin).apply_margin(_margin);
}
void   TextEdit::move_cursor(size_t where) {
    repaint();
    _cursor_pos = where;

    if (_multi) {
        // TODO : Support Multi
        return;
    }

    if (where == 0) {
        _offset = {0.0f, 0.0f};
        return;
    }

    auto [w, h] = _lay.size();

    auto text_rect = text_rectangle();
    auto txt_pos   = text_position();
    // Try text hint
    TextHitResults results;
    if (_lay.hit_test_range(0, where, txt_pos.x, txt_pos.y - h / 2, &results)) {
        auto box = results.back().box;
        auto cursor_x = box.x + box.w;
        auto cursor_y = box.y;

        BTK_LOG("[TextEdit] cursor_x.x = %f\n", cursor_x);
        BTK_LOG("[TextEdit] text_rect.top_right = %f\n", text_rect.top_right().x);

        if (!text_rect.contains(cursor_x, cursor_y)) {
            // Not contained
            BTK_LOG("[TextEdit] cursor is not on the box\n");
            if (cursor_x >= text_rect.top_right().x) {
                _offset.x += (text_rect.top_right().x - cursor_x);
            }
            else if (cursor_x <= text_rect.top_left().x){
                _offset.x += (text_rect.top_left().x - cursor_x);
            }

            // TODO : Buggy here

            BTK_LOG("[TextEdit] offset.x = %f\n", _offset.x);
        }
        // Center, and cursor is at text end
        else if (_cursor_pos == _text_len) {
            BTK_LOG("[TextEdit] Cursor is on text end\n");
            _offset.x = text_rect.w - box.w;
            if (_offset.x > 0) {
                _offset.x = 0;
            }
        }
    }
}

BTK_NS_END