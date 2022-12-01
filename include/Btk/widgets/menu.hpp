#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

/**
 * @brief Popuped widget, it is better to placee it at heap and set DeleteOnClose attribute
 * 
 */
class PopupWidget : public Widget {
    public:
        PopupWidget();
        ~PopupWidget();

        void popup(Widget *where = nullptr);
        
        bool paint_event(PaintEvent &) override;
    private:
        static bool Filter(Object *, Event &, void *self);

        Widget *attached_widget = nullptr;
};

class PopupMenu : public PopupWidget {
    
};

class MenuBar : public Widget {
    public:

    private:
        bool filter();
};

BTK_NS_END