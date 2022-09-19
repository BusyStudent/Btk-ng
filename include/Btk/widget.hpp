#pragma once

#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <Btk/string.hpp>
#include <Btk/style.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#include <bitset>
#include <list>


BTK_NS_BEGIN

class KeyEvent;
class DragEvent;
class MouseEvent;
class WheelEvent;
class FocusEvent;
class PaintEvent;
class ResizeEvent;
class MotionEvent;
class TextEditEvent;
class TextInputEvent;

// Enum for window flags
enum class WindowFlags : uint32_t {
    None = 0,
    Resizable  = 1 << 0,
    Fullscreen = 1 << 1,
    Borderless = 1 << 2,
    Hidden     = 1 << 3,
    Maximized  = 1 << 4,
    Minimized  = 1 << 5,
    OpenGL     = 1 << 6,
    Vulkan     = 1 << 7,
    AcceptDrop = 1 << 8,
    Transparent = 1 << 9
};
// Enum for widget.
enum class FocusPolicy : uint8_t {
    None = 0,       //< Widget does not accept focus.
    Mouse = 1 << 0, //< Widget accepts focus by mouse click.
};
enum class WidgetAttr  : uint8_t {
    DeleteOnClose = 0,
};
// Class for widget
class SizePolicy {
    public:
        enum Policy : uint8_t {
            GrowMask   = 1 << 0, //< Expand on needed
            ExpandMask = 1 << 1, //< Get space as big as possible
            ShrinkMask = 1 << 2, //< Shrink on needed
            IgnoreMask = 1 << 3,

            Fixed     = 0,
            Expanding = GrowMask | ShrinkMask | ExpandMask,
            Ignore    = GrowMask | ShrinkMask | IgnoreMask,
            Minimum   = GrowMask,
            Maximum   = ShrinkMask,
        };

        SizePolicy() = default;
        SizePolicy(Policy f) : v_flag(f), h_flag(f) {}
        SizePolicy(Policy v, Policy h) : v_flag(v), h_flag(h) {}

        Policy vertical_policy() const noexcept {
            return v_flag;
        }
        Policy horizontal_policy() const noexcept {
            return h_flag;
        }
        int    vertical_stretch() const noexcept {
            return v_stretch;
        }
        int    horizontal_stretch() const noexcept {
            return h_stretch;
        }
    private:
        Policy v_flag = Fixed;
        Policy h_flag = Fixed;
        int  v_stretch = 0;
        int  h_stretch = 0;
};

BTK_FLAGS_OPERATOR(SizePolicy::Policy, uint8_t);
BTK_FLAGS_OPERATOR(WindowFlags, uint32_t);
BTK_FLAGS_OPERATOR(FocusPolicy, uint8_t);

/**
 * @brief Widget base class
 * 
 */
class BTKAPI Widget : public Object {
    public:
        Widget();
        Widget(Widget *parent);
        ~Widget();

        // Configure
        void show();
        void hide();
        void raise();
        void lower();
        void close();
        void repaint();
        void repaint_now();

        void set_visible(bool visible);
        void set_rect(int x, int y, int w, int h);
        void move(int x, int y);

        void resize(int width, int height);
        void resize(const Size &p) {
            resize(p.w, p.h);
        }

        // Querys
        Rect    rect() const;
        bool    visible() const;
        Widget *parent() const;
        Widget *window() const;
        Widget *root() const;
        Style  *style() const;
        bool    is_window() const;
        bool    is_root() const;
        auto    font() const -> const Font &;

        int     width() const {
            return rect().w;
        }
        int     height() const {
            return rect().h;
        }
        int     x() const {
            return rect().x;
        }
        int     y() const {
            return rect().y;
        }

        Size    size() const {
            return rect().size();
        }

        Size    adjust_size() const;

        Size    maximum_size() const {
            return _maximum_size;
        }
        Size    minimum_size() const {
            return _minimum_size;
        }
        FocusPolicy focus_policy() const {
            return _focus;
        }
        SizePolicy  size_policy() const {
            return _size;
        }
        // Size Hints
        virtual Size size_hint() const;

        // Event handlers
        virtual bool handle       (Event &event) override;
        virtual bool key_press    (KeyEvent &) { return false; }
        virtual bool key_release  (KeyEvent &) { return false; }

        virtual bool mouse_press  (MouseEvent &) { return false; }
        virtual bool mouse_release(MouseEvent &) { return false; }
        virtual bool mouse_wheel  (WheelEvent &)  { return false; }
        virtual bool mouse_enter  (MotionEvent &) { return false; }
        virtual bool mouse_leave  (MotionEvent &) { return false; }
        virtual bool mouse_motion (MotionEvent &) { return false; }

        virtual bool drag_begin   (DragEvent &) { return false; }
        virtual bool drag_end     (DragEvent &) { return false; }
        virtual bool drag_motion  (DragEvent &) { return false; }

        virtual bool focus_gained (FocusEvent &) { return false; }
        virtual bool focus_lost   (FocusEvent &) { return false; }

        virtual bool textinput_event(TextInputEvent &) { return false; }
        virtual bool resize_event   (ResizeEvent &)    { return false; }
        virtual bool paint_event    (PaintEvent &)     { return false; }
        virtual bool change_event   (Event &)          { return false; }
        // virtual bool drop_event   (DropEvent &) { return false; }

        // Get window associated with widget
        window_t   winhandle() const;
        driver_t   driver()    const;
        Painter   &painter()   const;

        // Child widgets
        void       add_child(Widget *child);
        bool       has_child(Widget *child) const;
        void       remove_child(Widget *child);

        void       set_parent(Widget *parent);
        // Paint child widgets
        void       paint_children(PaintEvent &);

        // Locate child widget by point
        Widget    *child_at(int x, int y) const;
        Widget    *child_at(const Point &p) const {
            return child_at(p.x, p.y);
        }
        Widget    *child_at(u8string_view name) const;


        // Configure window
        void set_window_title(u8string_view title);
        void set_window_size(int width, int height);
        void set_window_position(int x, int y);
        void set_window_icon(const PixBuffer &icon);
        bool set_window_flags(WindowFlags flags);
        bool set_window_borderless(bool v);
        bool set_resizable(bool v);
        bool set_fullscreen(bool v);

        // Mouse
        void capture_mouse(bool capture = true);

        // Keyboard Text Input
        void set_textinput_rect(const Rect &rect);
        void start_textinput(bool start = true);
        void stop_textinput();

        // Configure properties
        void set_focus_policy(FocusPolicy policy);
        void set_size_policy(SizePolicy policy);
        void set_maximum_size(int w, int h);
        void set_minimum_size(int w, int h);
        void set_maximum_size(Size size);
        void set_minimum_size(Size size);
        void set_style(Style *style);
        void set_font(const Font &font);
    private:
        void window_init(); //< Create window if not created yet.
        void window_destroy(); //< Destroy window if created.

        UIContext  *_context    = nullptr; //< Pointer to UIContext
        Widget     *_parent     = nullptr; //< Parent widget
        Style      *_style      = nullptr; //< Pointer to style
        WindowFlags _flags      = WindowFlags::Resizable; //< Window flags
        FocusPolicy _focus      = FocusPolicy::None; //< Focus policy
        SizePolicy  _size       = SizePolicy::Fixed; //< Size policy
        std::list<Widget *>           _children; //< Child widgets
        std::list<Widget *>::iterator _in_child_iter = {}; //< Child iterator

        Size        _maximum_size = {INT_MAX, INT_MAX}; //< Maximum size
        Size        _minimum_size = {0, 0}; //< Minimum size
        Rect        _rect = {0, 0, 0, 0}; //< Rectangle
        Font        _font    = {}; //< Font

        window_t    _win     = {}; //< Window handle
        Painter     _painter = {}; //< Painter
        uint8_t     _painter_inited  = false; //< Is painter inited ?

        u8string    _name    = {}; //< Widget name

        // Attributes
        uint8_t     _accept_drop = false; //< Is drop accepted ?
        uint8_t     _visible     = true;  //< Visibility
        uint8_t     _enabled     = true;  //< Enabled
        uint8_t     _focused     = false; //< Has focused ?
        uint8_t     _drag_reject = false; //< Is drag rejected ?
        uint8_t     _pressed     = false; //< Is pressed ?

        // Event Dispatch (private)
        Widget      *mouse_current_widget = nullptr;//< Current widget under mouse
        Widget      *focused_widget       = nullptr;//< Focused widget (keyboard focus)
        Widget      *dragging_widget      = nullptr;//< Dragging widget
};


// Common useful widgets

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
        bool _checked = false;
        bool _pressed = false;
        bool _entered = false;
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
        bool _pressed = false;
        bool _entered = false;
};

// Label
class BTKAPI Label : public Widget {
    public:
        Label(Widget *parent = nullptr, u8string_view txt = {});
        Label(u8string_view txt) : Label(nullptr, txt) {}
        ~Label();

        void set_text(u8string_view txt);

        bool paint_event(PaintEvent &event) override;
        bool change_event(Event     &event) override;
        Size size_hint() const override; //< Return to text size
    private:
        TextLayout _layout;
        Alignment  _align = Alignment::Left | Alignment::Middle;
};

// Frame 
class BTKAPI Frame : public Widget {

};

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
        bool     _entered = false; //< Mouse entered ?
        timerid_t _timerid = 0;

        Signal<void()> _text_changed;
        Signal<void()> _enter_pressed;
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

// Slider & ScrollBar
class BTKAPI Slider : public Widget {
    public:
        Slider(Widget *parent = nullptr);
        ~Slider();

        void set_range(int64_t min, int64_t max);
        void set_value(int64_t value);

        void set_page_step(uint64_t step);
        void set_line_step(uint64_t step);
        void set_tracking(bool tracking);

        bool paint_event(PaintEvent &) override;

        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool mouse_wheel(WheelEvent &event) override;

        bool drag_begin(DragEvent &event) override;
        bool drag_motion(DragEvent &event) override;
        bool drag_end(DragEvent &event) override;

        Size size_hint() const override;

        int64_t value() const noexcept {
            return _value;
        }

        BTK_EXPOSE_SIGNAL(_value_changed);
        BTK_EXPOSE_SIGNAL(_range_changed);
        BTK_EXPOSE_SIGNAL(_slider_released);
        BTK_EXPOSE_SIGNAL(_slider_pressed);
        BTK_EXPOSE_SIGNAL(_slider_moved);
    private:
        FRect content_rect() const;
        FRect bar_rect()     const;

        bool    _dragging = false;
        bool    _pressed = false;
        bool    _hovered = false;

        int64_t _min = 0;
        int64_t _max = 100;
        int64_t _value = 0;

        uint64_t _page_step = 10;
        uint64_t _line_step = 1;

        Orientation _orientation = Horizontal;

        Signal<void()> _value_changed;
        Signal<void()> _range_changed;
        Signal<void()> _slider_released;
        Signal<void()> _slider_pressed;
        Signal<void()> _slider_moved;
};

// Display image
class ImageView : public Widget {
    public:
        ImageView(Widget *parent = nullptr, const PixBuffer *pix = nullptr);
        ImageView(const PixBuffer &pix) : ImageView(nullptr, &pix) {}
        ~ImageView();

        void set_image(const PixBuffer &img);
        void set_keep_aspect_ratio(bool keep);

        bool paint_event(PaintEvent &event) override;
        Size size_hint() const override;
    private:
        Texture  texture;
        PixBuffer pixbuf;

        float radius = 0;
        bool dirty = false;
        bool keep_aspect = false;
};



// Layout Tools
class SpacerItem ;
class LayoutItem {
    public:
        virtual ~LayoutItem() = default;

        virtual void mark_dirty()               = 0;
        virtual void set_rect(const Rect &rect) = 0;
        virtual Size size_hint() const = 0;
        virtual Rect rect()      const = 0;

        // Safe cast to
        virtual SpacerItem *spacer_item() {return nullptr;}
        virtual Widget *widget() const { return nullptr; }
        virtual Layout *layout() { return nullptr; }

        void set_alignment(Alignment alig) {
            _alignment = alig;
            mark_dirty();
        }
        auto alignment() const -> Alignment {
            return _alignment;
        }
    private:
        Alignment _alignment = {};
};
class WidgetItem : public LayoutItem {
    public:
        WidgetItem(Widget *w) : wi(w) {}

        void mark_dirty() override {
            
        }
        void set_rect(const Rect &r) override {
           wi->set_rect(r.x, r.y, r.w, r.h); 
        }
        Size size_hint() const override {
            return wi->size_hint();
        }
        Rect rect() const override {
            return wi->rect();
        }

        Widget *widget() const override {
            return wi;
        }
    private:
        Widget *wi;
};
class SpacerItem : public LayoutItem {
    public:
        SpacerItem(int w, int h) : size(w, h) {}

        void mark_dirty() override {
            
        }
        void set_rect(const Rect &r) override {
        
        }
        Size size_hint() const override {
            return size;
        }
        SpacerItem *spacer_item() override {
            return this;
        }

        Rect rect() const override {
            return Rect(0, 0, size.w, size.h);
        }

        void set_size(int w, int h) {
            size.w = w;
            size.h = h;
        }
    private:
        Size size;
};

class BTKAPI Layout     : public LayoutItem, public Trackable {
    public:
        Layout(Widget *parent = nullptr);
        ~Layout();

        void attach(Widget *widget);
        void set_margin(Margin m);
        void set_spacing(int spacing);
        void set_parent(Layout *lay);

        Margin margin() const {
            return _margin;
        }
        int    spacing() const {
            return _spacing;
        }
        Layout *parent() const {
            return _parent;
        }

        // Override
        Widget *widget() const override;
        Layout *layout() override;

        // Abstract Interface
        virtual void        add_item(LayoutItem *item) = 0;
        virtual int         item_index(LayoutItem *item) = 0;
        virtual LayoutItem *item_at(int idx) = 0;
        virtual LayoutItem *take_item(int idx) = 0;
        virtual int         count_items() = 0;
        virtual void        mark_dirty() = 0;
        virtual void        run_hook(Event &) = 0;
    private:
        Margin                 _margin = {0} ; //< Content margin
        Widget                *_widget = nullptr; //< Attached 
        Layout                *_parent = nullptr;
        int                    _spacing = 0;

        bool _hooked = false; //< Hooked to widget ? (default = false)
};
class BTKAPI BoxLayout : public Layout {
    public:
        BoxLayout(Direction d = LeftToRight);
        ~BoxLayout();

        void add_layout(Layout *lay, int stretch = 0);
        void add_widget(Widget *wid, int stretch = 0, Alignment align = {});
        void add_spacing(int spacing);
        void add_stretch(int stretch);
        void set_direction(Direction d);

        void        add_item(LayoutItem *item)   override;
        int         item_index(LayoutItem *item) override;
        LayoutItem *item_at(int idx)             override;
        LayoutItem *take_item(int idx)           override;
        int         count_items()                override;
        void        mark_dirty()                 override;
        void        run_hook(Event &)            override;
        void        set_rect(const Rect &)       override;
        Rect        rect()                 const override;
        Size        size_hint()            const override; 
    private:
        void        run_layout(const Rect *dst);

        struct ItemExtra {
            int stretch = 0;

            // Cached field in run_layout
            Size alloc_size = {0, 0};
        };

        mutable Size cached_size = {0, 0}; //< Measured size
        mutable bool size_dirty = true;

        std::vector<std::pair<LayoutItem*, ItemExtra>> items;
        Direction _direction = LeftToRight;
        Rect      _rect = {0, 0, 0, 0};
        bool _except_resize = false;
        bool _dirty = true;
        int  n_spacing_item = 0;
};


// Dialog
class Dialog : public Widget {
    public:
};

// ScrollArea
class AbstractScrollArea : public Widget {
    public:
        AbstractScrollArea(Widget *parent = nullptr);
        ~AbstractScrollArea();
};
class ScrollArea : public AbstractScrollArea {
    public:
        ScrollArea(Widget *parent = nullptr);
        ~ScrollArea();
};

// ListView MVC
class ItemModel {

};


// Inline methods for Widget
inline void Widget::stop_textinput() {
    start_textinput(false);
}
inline void Widget::set_maximum_size(Size s) {
    set_maximum_size(s.w, s.h);
}
inline void Widget::set_minimum_size(Size s) {
    set_minimum_size(s.w, s.h);
}

BTK_NS_END