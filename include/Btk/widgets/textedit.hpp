#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

// TextEdit
// TODO : Auto-wrap and move txt pos if needed
class BTKAPI TextEdit : public Widget {
    public:
        TextEdit(Widget *parent = nullptr, u8string_view text = {});
        TextEdit(u8string_view text) : TextEdit(nullptr, text) {}
        ~TextEdit();

        void set_text(u8string_view text);
        void set_placeholder(u8string_view text);

        bool has_selection() const;

        // Event handlers
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool key_press(KeyEvent &event) override;

        bool drag_begin(DragEvent &event) override;
        bool drag_motion(DragEvent &event) override;
        bool drag_end(DragEvent &event) override;

        bool focus_gained(FocusEvent &event) override;
        bool focus_lost(FocusEvent &event) override;

        bool timer_event(TimerEvent &event) override;
        bool paint_event(PaintEvent &event) override;
        bool textinput_event(TextInputEvent &event) override;
        bool change_event(Event &) override;

        Size size_hint() const override;

        u8string_view text() const {
            return _text;
        }
        u8string      selection_text() const {
            auto [start, end] = sel_range();
            return _text.substr(start, end - start);
        }

        BTK_EXPOSE_SIGNAL(_text_changed);
        BTK_EXPOSE_SIGNAL(_enter_pressed);
    private:
        size_t get_pos_from(const Point &pos);
        // Operation on selection
        void   do_paste(u8string_view txt);
        void   do_delete(size_t start, size_t end);
        void   clear_sel();
        auto   sel_range() const -> std::pair<size_t,size_t> {
            size_t start = min(_sel_begin, _sel_end);
            size_t end   = max(_sel_begin, _sel_end);
            return {start,end};
        }

        FMargin _margin; //< Border margin
        FPoint  _txt_pos; //< Text position

        Alignment _align = Alignment::Left | Alignment::Middle; //< Text alignment

        u8string _placeholder; //< Placeholder text
        u8string _text; //< Text

        TextLayout _lay; //< Text Layout for analysis

        //  H e l l o W o r l d
        //0 1 2 3 4 5 6 7 8 9 10
        size_t   _sel_begin = 0; //< Selection begin
        size_t   _sel_end   = 0;   //< Selection end
        size_t   _cursor_pos = 0; //< Cursor position
        bool     _has_sel   = false; //< Has selection ?
        bool     _has_focus = false; //< Has focus ?
        bool     _show_cursor = false; //< Show cursor ?
        timerid_t _timerid = 0;

        Signal<void()> _text_changed;
        Signal<void()> _enter_pressed;
};

BTK_NS_END
