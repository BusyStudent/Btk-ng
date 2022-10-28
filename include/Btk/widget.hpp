#pragma once

#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <Btk/string.hpp>
#include <Btk/style.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#include <climits>
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
        auto    palette() const -> const Palette &;
        auto    palette_group() const -> Palette::Group;

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

        // Query window
        FPoint     window_dpi() const;

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
        bool under_mouse() const;

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
        void rectangle_update(); //< rectangle is updated.

        UIContext  *_context    = nullptr; //< Pointer to UIContext
        Widget     *_parent     = nullptr; //< Parent widget
        Style      *_style      = nullptr; //< Pointer to style
        WindowFlags _flags      = WindowFlags::Resizable; //< Window flags
        FocusPolicy _focus      = FocusPolicy::None; //< Focus policy
        SizePolicy  _size       = SizePolicy::Fixed; //< Size policy
        Palette     _palette    = {}; //< Palette of widget     
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
        uint8_t     _entered     = false; //< Is entered ?

        // Event Dispatch (private)
        Widget      *mouse_current_widget = nullptr;//< Current widget under mouse
        Widget      *focused_widget       = nullptr;//< Focused widget (keyboard focus)
        Widget      *dragging_widget      = nullptr;//< Dragging widget
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
inline bool Widget::under_mouse() const {
    return _entered;
}

// Inline helper function
template <typename T>
inline T    PixelToIndependent(T value, float dpi) BTK_NOEXCEPT_IF(value / std::round(dpi / 96.0f)) {
    return value / std::round(dpi / 96.0f);
}
template <typename T>
inline T    IndependentToPixel(T value, float dpi) BTK_NOEXCEPT_IF(value * std::round(dpi / 96.0f)) {
    return value * std::round(dpi / 96.0f);
}

BTK_NS_END