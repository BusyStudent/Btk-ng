#pragma once

#include "Btk/string.hpp"
#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI Action : public Object {
    public:
        Action();
        ~Action();
    private:
        PixBuffer _image;
        u8string  _text;
};

/**
 * @brief Popuped widget, it is better to placee it at heap and set DeleteOnClose attribute
 * 
 */
class BTKAPI PopupWidget : public Widget {
    public:
        PopupWidget();
        ~PopupWidget();

        void popup(Widget *where = nullptr);
        
        bool paint_event(PaintEvent &) override;
    private:
        static bool Filter(Object *, Event &, void *self);

        Widget *attached_widget = nullptr;
};

class BTKAPI PopupMenu : public PopupWidget {
    
};

class BTKAPI MenuBar : public Widget {
    public:

    private:
        bool filter();
};

BTK_NS_END