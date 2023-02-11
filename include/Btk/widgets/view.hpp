#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class ScrollBar;

// Label
class BTKAPI Label : public Widget {
    public:
        Label(Widget *parent = nullptr, u8string_view txt = {});
        Label(u8string_view txt) : Label(nullptr, txt) {}
        ~Label();

        void set_text(u8string_view txt);

        bool paint_event(PaintEvent   &event) override;
        bool change_event(ChangeEvent &event) override;
        Size size_hint() const override; //< Return to text size
    private:
        TextLayout _layout;
        Alignment  _align = Alignment::Left | Alignment::Middle;
};

// Display image
class BTKAPI ImageView : public Widget {
    public:
        ImageView(Widget *parent = nullptr, const PixBuffer *pix = nullptr);
        ImageView(const PixBuffer &pix) : ImageView(nullptr, &pix) {}
        ~ImageView();

        void set_image(const PixBuffer &img);
        void set_image(const Image     &img);
        void set_keep_aspect_ratio(bool keep);

        bool paint_event(PaintEvent &event) override;
        bool timer_event(TimerEvent &event) override;
        // bool drop_event(DropEvent   &event) override;
        Size size_hint() const override;
    private:
        Texture  texture;
        PixBuffer pixbuf;
        Image     image;

        timerid_t timer = 0; //< Timer for refresh
        int       frame = 0; //< Current frame
        int       delay = 0; //< Current delay (for reuse timer)

        float radius = 0;
        bool dirty = false;
        bool keep_aspect = false;
};

// ProgressBar
class BTKAPI ProgressBar : public Widget {
    public:
        ProgressBar(Widget *parent = nullptr);
        ~ProgressBar();

        void set_value(int64_t value);
        void set_range(int64_t min, int64_t max);
        void set_direction(direction_t d);
        void set_text_visible(bool visible);
        void reset(); //< Reset to min value

        int64_t value() const {
            return _value;
        }

        bool paint_event(PaintEvent &event) override;

        BTK_EXPOSE_SIGNAL(_value_changed);
        BTK_EXPOSE_SIGNAL(_range_changed);
    private:
        int64_t _min = 0;
        int64_t _max = 100;
        int64_t _value = 0;

        u8string  _format = "%p %";

        Direction _direction = LeftToRight;
        bool      _text_visible = false;

        Signal<void()> _value_changed;
        Signal<void()> _range_changed;
};

/**
 * @brief Item used by ListBox (string + font)
 * 
 */
class ListItem {
    public:
        u8string text;
        Font     font;
    private:
        TextLayout layout; //< TextLayout of it
    friend class ListBox;
};
/**
 * @brief A ListBox of strings representing
 * 
 */
class ListBox : public Widget {
    public:
        ListBox(Widget *parent = nullptr);
        ~ListBox();
        /**
         * @brief Insert item to index
         * 
         * @param idx 
         * @param item 
         */
        void insert_item(size_t idx, const ListItem &item);
        /**
         * @brief Add item to end of the items list
         * 
         * @param item 
         */
        void add_item(const ListItem &item);
        /**
         * @brief Remove item
         * 
         * @param item 
         */
        void remove_item(ListItem *item);
        /**
         * @brief Scroll to item at 
         * 
         * @param item 
         */
        void scroll_to(ListItem *item);
        /**
         * @brief Hide the border
         * 
         * @param v True on hide
         */
        void set_flat(bool v);
        /**
         * @brief Clear all items
         * 
         */
        void clear();

        /**
         * @brief Get item at (x, y) in screen coord
         * 
         * @param x 
         * @param y 
         * @return ListItem* 
         */
        ListItem *item_at(float x, float y);
        /**
         * @brief Get item at positon
         * 
         * @param pos The position of item 
         * @return ListItem* nullptr on invalid
         */
        ListItem *item_at(int pos);
        /**
         * @brief Current Item selected
         * 
         * @return ListItem* 
         */
        ListItem *current_item();
        /**
         * @brief Get index of item
         * 
         * @param item pointer to item
         * @return int (-1 on not invalid or nullptr)
         */
        int       index_of(const ListItem *item) const;

        BTK_EXPOSE_SIGNAL(_current_item_changed);
        BTK_EXPOSE_SIGNAL(_item_clicked);
        BTK_EXPOSE_SIGNAL(_item_enter);
    protected:
        bool paint_event(PaintEvent &event) override;
        bool resize_event(ResizeEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_wheel(WheelEvent &event) override;
        bool mouse_motion(MotionEvent &event) override;
        bool focus_gained(FocusEvent &event) override;
        bool focus_lost(FocusEvent &event) override;
    private:        
        void set_current_item(ListItem *item);
        /**
         * @brief Set the mouse hover object by position we gived
         * 
         * @param where 
         */
        void set_mouse_hover(Point where);
        void items_changed(); //< Items changed, need calc bounds
        /**
         * @brief Calc the slider should hide or not
         * 
         */
        void calc_slider();
        void vslider_value_changed();
        void hslider_value_changed();

        std::vector<ListItem> _items;
        ScrollBar            *_vslider = nullptr;
        ScrollBar            *_hslider = nullptr;
        int                   _current = -1; //< Current selected 
        int                   _hovered = -1; //< Current mouse hovered

        float                 _ytranslate = 0.0f;
        float                 _xtranslate = 0.0f;

        bool                  _flat = false;

        Signal<void()>           _current_item_changed;
        Signal<void(ListItem *)> _item_clicked;
        Signal<void(ListItem *)> _item_enter;
};
BTK_NS_END