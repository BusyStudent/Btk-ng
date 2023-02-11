#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI ComboBox : public Widget {
    public:
        ComboBox(Widget *parent = nullptr);
        ~ComboBox();

        void add_item(u8string str);
        void remove_item(u8string_view what);
    private:
        StringList _items;
        Widget *_listbox;
};

BTK_NS_END