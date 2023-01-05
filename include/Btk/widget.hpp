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
class MoveEvent;
class MouseEvent;
class CloseEvent;
class WheelEvent;
class FocusEvent;
class PaintEvent;
class ChangeEvent;
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
    Transparent = 1 << 9,
    NeverShowed = 1 << 10,
};
// Enum for widget.
enum class FocusPolicy : uint8_t {
    None = 0,       //< Widget does not accept focus.
    Mouse = 1 << 0, //< Widget accepts focus by mouse click.
};
enum class WidgetAttrs : uint8_t {
    None          = 0, //< No attributes
    Debug         = 1 << 0 , //< Show debug info
    DeleteOnClose = 1 << 1, //< Widget Delete after closed
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
BTK_FLAGS_OPERATOR(WidgetAttrs, uint8_t);

/**
 * @brief Widget base class
 * 
 */
class BTKAPI Widget : public Object {
    public:
        /**
         * @brief Construct a new Widget object
         * 
         * @param parent The parent pointer (can be nullptr)
         */
        Widget(Widget *parent = nullptr);
        ~Widget();

        // Configure

        /**
         * @brief Make the widget visable
         * 
         */
        void show();
        /**
         * @brief Make the widget invisable
         * 
         */
        void hide();
        /**
         * @brief Raise the widget to top of the stack
         * 
         */
        void raise();
        /**
         * @brief Place the widget into bottom of the stack
         * 
         */
        void lower();
        /**
         * @brief Close the widget
         * 
         */
        void close();
        /**
         * @brief Send a paint event into queue
         * 
         */
        void repaint();
        /**
         * @brief Force repaint right now
         * 
         */
        void repaint_now();
        /**
         * @brief Try let the widget has the focus
         * 
         */
        void take_focus();
        /**
         * @brief Set the visible object
         * 
         * @param visible The visable
         */
        void set_visible(bool visible);
        /**
         * @brief Set the rect object
         * 
         * @param x 
         * @param y 
         * @param w 
         * @param h 
         */
        void set_rect(int x, int y, int w, int h);
        /**
         * @brief Set the palette object
         * 
         * @param palette The new color palette
         */
        void set_palette(const Palette &palette);
        /**
         * @brief Move the widget to position x, y
         * 
         * @param x 
         * @param y 
         */
        void move(int x, int y);
        /**
         * @brief Resize the widget to width, height
         * 
         * @param width 
         * @param height 
         */
        void resize(int width, int height);
        /**
         * @brief Resize the widget to (Size ver) 
         * 
         * @param p 
         */
        void resize(const Size &p) {
            resize(p.w, p.h);
        }

        // Querys
        /**
         * @brief Get the widget rectangle
         * 
         * @return Rect 
         */
        Rect    rect() const;
        /**
         * @brief Get the widget visiblity
         * 
         * @return true 
         * @return false 
         */
        bool    visible() const;
        /**
         * @brief Get the pointer of the parent
         * 
         * @return Widget* (nullptr on no parent)
         */
        Widget *parent() const;
        /**
         * @brief Get the window of the widget tree 
         * 
         * @return Widget* (nullptr on on window)
         */
        Widget *window() const;
        /**
         * @brief Get the root widget of the widget tree
         * 
         * @return Widget* 
         */
        Widget *root() const;
        /**
         * @brief Get the style pointer of the widget
         * 
         * @return Style* 
         */
        Style  *style() const;
        /**
         * @brief Check current widget is window
         * 
         * @return true 
         * @return false 
         */
        bool    is_window() const;
        /**
         * @brief Check current widget is root widget
         * 
         * @return true 
         * @return false 
         */
        bool    is_root() const;
        /**
         * @brief Check current widget has focus now
         * 
         * @return true 
         * @return false 
         */
        bool    has_focus() const;
        /**
         * @brief Get font of the widget
         * 
         * @return const Font& 
         */
        auto    font() const -> const Font &;
        /**
         * @brief Get color patette of the widget
         * 
         * @return const Palette& 
         */
        auto    palette() const -> const Palette &;
        /**
         * @brief Set the palette current group object
         * 
         * @param group the group object
         */
        auto    set_palette_current_group(Palette::Group) -> void;
        /**
         * @brief Get width of the widget
         * 
         * @return int 
         */
        int     width() const {
            return rect().w;
        }
        /**
         * @brief Get height of the widget
         * 
         * @return int 
         */
        int     height() const {
            return rect().h;
        }
        /**
         * @brief Get x position of the widget
         * 
         * @return int 
         */
        int     x() const {
            return rect().x;
        }
        /**
         * @brief Get y position of the widget
         * 
         * @return int 
         */
        int     y() const {
            return rect().y;
        }
        /**
         * @brief Get the size of the widget
         * 
         * @return Size 
         */
        Size    size() const {
            return rect().size();
        }
        /**
         * @brief Adjust the size of widget
         * 
         * @return Size 
         */
        Size    adjust_size() const;

        /**
         * @brief Get the maximum size of the widget
         * 
         * @return Size 
         */
        Size    maximum_size() const {
            return _maximum_size;
        }
        /**
         * @brief Get the minimum size of the widget
         * 
         * @return Size 
         */
        Size    minimum_size() const {
            return _minimum_size;
        }
        /**
         * @brief Get the size policy (used in gain focus)
         * 
         * @return FocusPolicy 
         */
        FocusPolicy focus_policy() const {
            return _focus;
        }
        SizePolicy  size_policy() const {
            return _size;
        }

        // Size Hints
        /**
         * @brief Virtual method for get perfered size of widget
         * 
         * @return Pefered size (default in Size(0, 0))
         */
        virtual Size size_hint() const;

        // Event handlers
        /**
         * @brief Process event
         * 
         * @param event The event reference
         * @return true On processed
         * @return false On unprocessed
         */
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
        virtual bool move_event     (MoveEvent  &)     { return false; }
        virtual bool paint_event    (PaintEvent &)     { return false; }
        virtual bool close_event    (CloseEvent &)     { return false; }
        virtual bool change_event   (ChangeEvent &)    { return false; }
        // virtual bool drop_event   (DropEvent &) { return false; }

        /**
         * @brief Get window associated with widget
         * 
         * @return window_t 
         */
        window_t   winhandle() const;
        /**
         * @brief Get the driver associated with widget
         * 
         * @return driver_t 
         */
        driver_t   driver()    const;
        /**
         * @brief Get the painter associated with widget
         * 
         * @return Painter& 
         */
        Painter   &painter()   const;

        // Child widgets
        /**
         * @brief Add a child
         * 
         * @param child The pointer of widget (nullptr on no-op)
         */
        void       add_child(Widget *child);
        /**
         * @brief Check widget is the child of the widget
         * 
         * @param child 
         * @return true 
         * @return false 
         */
        bool       has_child(Widget *child) const;
        /**
         * @brief Remove a child, it will remove the widget's ownship of the child
         * 
         * @param child Child pointer (nullptr on no-op)
         */
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

        
        void set_attribute(WidgetAttrs attr, bool on);

        // Mouse
        Point mouse_position() const;
        void  capture_mouse(bool capture = true);
        bool  under_mouse() const;
        bool  warp_mouse(int x, int y);

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
        void debug_draw(); //< Draw the debug info

        UIContext  *_context    = nullptr; //< Pointer to UIContext
        Widget     *_parent     = nullptr; //< Parent widget
        Style      *_style      = nullptr; //< Pointer to style
        WindowFlags _flags      = WindowFlags::Resizable; //< Window flags
        WidgetAttrs _attrs      = WidgetAttrs::None; //< Widget attributrs
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
inline T    PixelToIndependent(T value, float dpi) BTK_NOEXCEPT_IF(muldiv<T>(value, dpi, 96)) {
    return muldiv<T>(value, dpi, 96);
}
template <typename T>
inline T    IndependentToPixel(T value, float dpi) BTK_NOEXCEPT_IF(muldiv<T>(value, 96, dpi)) {
    return muldiv<T>(value, 96, dpi);
}

BTK_NS_END