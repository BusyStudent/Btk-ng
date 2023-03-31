#pragma once

#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI Action : public Object {
    public:
        Action();
        ~Action();

        PixBuffer image() const {
            return _image;
        }
        u8string_view text() const {
            return _text;
        }
    private:
        PixBuffer _image;
        u8string  _text;
};
class BTKAPI Menu    : public Widget {
    public:
        Menu(Widget *parent);
        ~Menu();
    private:
        std::vector<Action *> actions;
};

/**
 * @brief Popuped widget, it is better to placee it at heap and set DeleteOnClose attribute
 * 
 */
class BTKAPI PopupWidget : public Widget {
    public:
        PopupWidget();
        ~PopupWidget();

        /**
         * @brief Popup widget at where
         * 
         * @param where The widget for locking up the root widget, nullptr on PopupWindow
         */
        void popup(Widget *where = nullptr);
        /**
         * @brief Set the hide on focus lost, It will replace the behaivor of the widget when focus is lost
         * 
         * @param v 
         */
        void set_hide_on_focus_lost(bool v);
    public:
        // Signals
        BTK_EXPOSE_SIGNAL(_focus_lost);
    protected:
        bool mouse_press(MouseEvent &) override;
        bool mouse_release(MouseEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool focus_lost(FocusEvent &) override;
    private:
        bool hide_on_focus_lost = false;

        Signal<void()> _focus_lost;
};

class BTKAPI PopupMenu : public PopupWidget {
    
};

class BTKAPI MenuBar : public Widget {
    public:

    private:
        bool filter();
};

inline void PopupWidget::set_hide_on_focus_lost(bool v) {
    hide_on_focus_lost = v;
}

BTK_NS_END