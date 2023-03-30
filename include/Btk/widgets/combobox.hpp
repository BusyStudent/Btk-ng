#pragma once

#include <Btk/widget.hpp>
#include <Btk/widgets/menu.hpp>
#include <Btk/widgets/view.hpp>

BTK_NS_BEGIN

class BTKAPI ComboBox : public Widget {
    public:
        ComboBox(Widget *parent = nullptr);
        ~ComboBox();

        int max_visible_items() const;
        void set_max_visible_items(int max_items);

        int count() const;

        int find_text(u8string_view text);

        int current_index() const;
        u8string current_text() const;

        bool set_item_text(int index, u8string_view text);

        void add_item(u8string_view str);
        void remove_item(u8string_view what);

    public: // Event handlers
        bool paint_event(PaintEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool mouse_motion(MotionEvent &event) override;
		bool mouse_wheel(WheelEvent &event) override;
        bool change_event(ChangeEvent &event) override;

        Size size_hint() const override;

    public: // SLOTS
        void set_current_index(int index);
        void set_current_text(u8string_view text);
		void set_current_item(ListItem* item);


    public: // SIGNALS
        BTK_EXPOSE_SIGNAL(_text_changed);
        BTK_EXPOSE_SIGNAL(_current_text_changed);
        BTK_EXPOSE_SIGNAL(_current_index_changed);

    protected:
        Signal<void(int, u8string_view)> _text_changed;
        Signal<void(u8string_view)> _current_text_changed;
        Signal<void(int)> _current_index_changed;

    private:
        StringList _items;
        int _current_index = 0;
        int _max_visible_items = 5;
        TextLayout _text_layout;
		Alignment  _align = Alignment::Left | Alignment::Middle;
        ListBox *_listbox_view = nullptr;
		PopupWidget _popup_widget;
};

BTK_NS_END