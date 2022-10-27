#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI AbstractButton : public Widget {
    public:
        using Widget::Widget;

        void set_text(u8string_view us) {
            _text = us;
            repaint();
        }
        void set_flat(bool flat) {
            _flat = flat;
            repaint();
        }
        auto text() const -> u8string_view {
            return _text;
        }

        // Signals
        BTK_EXPOSE_SIGNAL(_howered);
        BTK_EXPOSE_SIGNAL(_clicked);
    protected:

        u8string _text;
        Brush    _icon; //< Button icon (if any)
        bool     _flat = false;

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
        RadioButton(Widget *parent = nullptr);
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
        bool _pressed = false;

        Priv *_priv = nullptr;
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

        Size size_hint() const override;
    private:
        class Priv;

        bool  _pressed = false;
        Priv *_priv    = nullptr;
};

BTK_NS_END