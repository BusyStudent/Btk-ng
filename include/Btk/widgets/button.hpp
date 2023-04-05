#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI AbstractButton : public Widget {
    public:
        AbstractButton(Widget *parent = nullptr);

        void set_flat(bool flat);
        void set_text(u8string_view us);
        void set_text_align(Alignment align);
        void set_icon(const PixBuffer &icon);
        void set_icon_size(const Size &size);
        auto text() const -> u8string_view {
            return _text;
        }

        // Signals
        BTK_EXPOSE_SIGNAL(_howered);
        BTK_EXPOSE_SIGNAL(_clicked);
    protected:
        /**
         * @brief Paint icon & text at limited area
         * 
         */
        void paint_icon_text(const FRect &where);
        bool paint_event(PaintEvent &) override = 0; //< Let it become a abstract class
        bool change_event(ChangeEvent &) override;

        Alignment  _textalign = AlignLeft | AlignMiddle;
        TextLayout _textlay; //< The text layout of this text
        u8string _text;      //< The text
        Brush    _icon;      //< Button icon (if any)
        Size     _icon_size; //< The size of the Icon (if any)
        bool     _flat = false;
        bool     _has_icon = false;
        bool     _pressed = false;

        Signal<void()> _howered;
        Signal<void()> _clicked;

};

// class BTKAPI CheckButton : public AbstractButton {
//     public:
//         CheckButton(Widget *parent = nullptr);
//         ~CheckButton();

//         void set_checked(bool checked);
//         bool checked() const;
// };

class BTKAPI RadioButton : public AbstractButton {
    public:
        RadioButton(Widget *parent = nullptr, u8string_view txt = {});
        RadioButton(u8string_view txt) : RadioButton(nullptr, txt) {}
        ~RadioButton();

        void set_checked(bool checked) {
            _checked = checked;
            repaint();
        }
        bool checked() const {
            return _checked;
        }

        // Event handlers
        bool paint_event(PaintEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;

        Size size_hint() const override;
    private:
        class Priv;

        bool _checked = false;

        Priv *_priv = nullptr;
};
class BTKAPI CheckButton : public AbstractButton {
    public:
        CheckButton();
};
class BTKAPI Button      : public AbstractButton {
    public:
        Button(Widget *parent = nullptr, u8string_view txt = {});
        Button(u8string_view txt) : Button(nullptr, txt) {}
        ~Button();

        // Event handlers
        bool paint_event(PaintEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool focus_gained(FocusEvent &event) override;
        bool focus_lost(FocusEvent &event) override;

        Size size_hint() const override;
    private:
        class Priv;

        Priv *_priv    = nullptr;
};

BTK_NS_END