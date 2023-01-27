#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

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

class ListItem {
    public:
        u8string text;
        Font     font;
};
/**
 * @brief A ListBox of strings representing
 * 
 */
class ListBox : public Widget {
    public:
        ListBox(Widget *parent = nullptr);
        ~ListBox();

        bool paint_event(PaintEvent &event) override;
        bool resize_event(ResizeEvent &event) override;


        void insert_item(size_t idx, const ListItem &item);
        void add_item(const ListItem &item);

        /**
         * @brief Current Item selected
         * 
         * @return ListItem* 
         */
        ListItem *current_item() const;

        BTK_EXPOSE_SIGNAL(_current_item_changed);
        BTK_EXPOSE_SIGNAL(_item_clicked);
        BTK_EXPOSE_SIGNAL(_item_enter);
    private:
        std::vector<ListItem> _items;
        Widget               *_vslider = nullptr;

        Signal<void()>        _current_item_changed;
        Signal<void()>        _item_clicked;
        Signal<void()>        _item_enter;
        Signal<void()>        _item_leave;
};

BTK_NS_END