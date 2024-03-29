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

// SDL Capicity SystemCursor enums
enum class SystemCursor : uint32_t {
    Arrow,    
    Ibeam,    
    Wait,      
    Crosshair, 
    WaitArrow, 
    SizeNwse,  
    SizeNesw, 
    SizeWe,    
    SizeNs,    
    SizeAll,   
    No,        
    Hand,      
    NumOf
};

// Enum for window flags
enum class WindowFlags : uint32_t {
    None       = 0,
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
    Framebuffer = 1 << 10, //< This window is used as a framebuffer
    PopupMenu   = 1 << 11, //< Window is a pop-up (ie. a window manager) window (e.g. Cocoa)
};
// Enum for widget.
enum class FocusPolicy : uint8_t {
    None  = 0,       //< Widget does not accept focus.
    Mouse = 1 << 0, //< Widget accepts focus by mouse click.
};
enum class WidgetAttrs : uint8_t {
    None          = 0, //< No attributes
    Debug         = 1 << 0, //< Show debug info
    DeleteOnClose = 1 << 1, //< Widget Delete after closed
    ClipRectangle = 1 << 2, //< Clip rect, avoid draw out of there  
    BackgroundTransparent = 1 << 3, //< Make background transparent
    PaintBackground = 1 << 4, //< Force widget paint it's background even is not on the top
    PaintChildren   = 1 << 5, //< Paint children widget (default on)
    MouseTransparent = 1 << 7, //< Mouse event will through it
};
enum class SizeHint    : uint8_t {
    Perfered = 0,
    Minimum  = 1
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
        void   set_vertical_stretch(int stretch) noexcept { 
            v_stretch = stretch;
        }
        void   set_horizontal_stretch(int stretch) noexcept { 
            h_stretch = stretch; 
        }
        void   set_vertical_policy(Policy p) noexcept {
            v_flag = p;
        }
        void   set_horizontal_policy(Policy p) noexcept {
            h_flag = p;
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
 * @brief Mouse cursor interface
 * 
 */
class BTKAPI Cursor {
    public:
        Cursor();
        Cursor(SystemCursor syscursor);
        Cursor(const PixBuffer &image, int hot_x, int hot_y);
        Cursor(const Cursor &);
        ~Cursor();

        Cursor &operator =(const Cursor &);

        void set() const;
        bool empty() const;
    private:
        Ref<AbstractCursor> cursor;
};

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
        /**
         * @brief Construct a new Widget object
         * 
         * @param parent The parent pointer (can be nullptr)
         * @param winflags The window flags
         */
        Widget(Widget *parent, WindowFlags winflags);
        /**
         * @brief Destroy the Widget object
         * 
         */
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
         * @brief Set the rect object (Point ver) 
         * 
         * @param p 
         */
        void set_rect(const Rect &r) {
            set_rect(r.x, r.y, r.w, r.h);
        }
        /**
         * @brief Set the palette object
         * 
         * @param palette The new color palette
         */
        void set_palette(const Palette &palette);
        /**
         * @brief Set the name object
         * 
         * @param name 
         */
        void set_name(u8string_view name);
        /**
         * @brief Move the widget to position x, y
         * 
         * @param x 
         * @param y 
         */
        void move(int x, int y);
        /**
         * @brief Move the widget to position x, y (Point ver) 
         * 
         * @param p 
         */
        void move(const Point &p) {
            move(p.x, p.y);
        }
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
         * @brief Get the cursor of the widget
         * 
         * @return const Cursor& 
         */
        auto    cursor() const -> const Cursor &;
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
         * @brief Get the opacity of the widget
         * 
         * @return float 
         */
        float   opacity() const {
            return _opacity;
        }
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
         * @brief Get the position of the widget
         * 
         * @return Point 
         */
        Point   position() const {
            return rect().position();
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
        /**
         * @brief Get the size policy
         * 
         * @return SizePolicy 
         */
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
        virtual bool handle(Event &event) override;
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
        /**
         * @brief Set the parent object
         * 
         * @param parent The parent ptr (can be nullptr)
         */
        void       set_parent(Widget *parent);
        /**
         * @brief Pain children 
         * 
         * @param event PaintEvent
         * 
         */
        void       paint_children(PaintEvent &);

        /**
         * @brief Locate child by position
         * 
         * @param x 
         * @param y 
         * @return Widget* nullptr on not-founded
         */
        Widget    *child_at(int x, int y) const;
        /**
         * @brief Locate child by position Point-ver
         * 
         * @param p 
         * @return Widget* 
         */
        Widget    *child_at(const Point &p) const {
            return child_at(p.x, p.y);
        }
        /**
         * @brief Locate child by its name
         * 
         * @param name 
         * @return Widget* 
         */
        Widget    *child_at(u8string_view name) const;
        /**
         * @brief Get the window dpi
         * 
         * @return FPoint 
         */
        FPoint     window_dpi() const;
        /**
         * @brief Convert screen point to client 
         * 
         * @param x 
         * @param y 
         * @return Point 
         */
        Point      map_to_client(int x, int y) const;
        /**
         * @brief Convert client point to screen
         * 
         * @param x 
         * @param y 
         * @return Point 
         */
        Point      map_to_screen(int x, int y) const;
        /**
         * @brief Convert device independent pixels (DIPs) to physical pixels
         * 
         * @param x 
         * @param y 
         * @return Point 
         */
        Point      map_to_pixels(int x, int y) const;
        /**
         * @brief Convert physical pixels to device independent pixels (DIPs)
         * 
         * @param x 
         * @param y 
         * @return Point 
         */
        Point      map_to_dips(int x, int y) const;

        Point      map_to_client(const Point &p) const {
            return map_to_client(p.x, p.y);
        }
        Point      map_to_screen(const Point &p) const {
            return map_to_screen(p.x, p.y);
        }
        Point      map_to_pixels(const Point &p) const {
            return map_to_pixels(p.x, p.y);
        }
        Point      map_to_dips(const Point &p) const {
            return map_to_dips(p.x, p.y);
        }
        Size       map_to_pixels(const Size &s) const {
            auto [x, y] = map_to_pixels(s.w, s.h);
            return Size(x, y);
        }
        Size       map_to_dips(const Size &s) const {
            auto [x, y] = map_to_dips(s.w, s.h);
            return Size(x, y);
        }

        /**
         * @brief Map a position from parent to self coord
         * 
         * @param parent_p The position in parent coord system
         * @return Point 
         */
        Point      map_from_parent_to_self(const Point &parent_p) const {
            Point result = parent_p;
            result -= position();
            return result;
        }
        /**
         * @brief Map a posiition from self to parent coord
         * 
         * @param self_p The position in self coord system
         * @return Point 
         */
        Point      map_from_self_to_parent(const Point &self_p) const {
            Point result = self_p;
            result += position();
            return result;
        }
        Point      map_from_self_to_root(const Point &) const;
        Point      map_from_root_to_self(const Point &) const;

        Point      map_from_self_to_screen(const Point &p) const {
            return map_to_screen(map_from_self_to_root(p));
        }

        // Configure window
        void set_window_title(u8string_view title);
        void set_window_icon(const PixBuffer &icon);
        bool set_window_flags(WindowFlags flags);
        bool set_window_borderless(bool v);
        bool set_resizable(bool v);
        bool set_fullscreen(bool v);

        // Attribute
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

        // Layout
        void request_layout();

        // Configure properties
        void set_focus_policy(FocusPolicy policy);
        void set_size_policy(SizePolicy policy);
        void set_maximum_size(int w, int h);
        void set_minimum_size(int w, int h);
        void set_maximum_size(Size size);
        void set_minimum_size(Size size);
        void set_fixed_size(int w, int h);
        void set_fixed_size(Size size);
        void set_style(Style *style);
        void set_font(const Font &font);
        void set_cursor(const Cursor &cursor);
        void set_opacity(float opacity);

        // Static Helper
        static Widget *KeyboardFocusWidget();
        static Widget *MouseFocusWidget();
    protected:
        virtual bool key_press    (KeyEvent &) { return false; }
        virtual bool key_release  (KeyEvent &) { return false; }

        //< Default, all mouse event will not through it
        virtual bool mouse_press  (MouseEvent &) { return true; } 
        virtual bool mouse_release(MouseEvent &) { return true; }
        virtual bool mouse_wheel  (WheelEvent &)  { return true; }
        virtual bool mouse_enter  (MotionEvent &) { return true; }
        virtual bool mouse_leave  (MotionEvent &) { return true; }
        virtual bool mouse_motion (MotionEvent &) { return true; }

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
    private:
        void window_init(); //< Create window if not created yet.
        void window_destroy(); //< Destroy window if created.
        void rectangle_update(); //< rectangle is updated.
        void debug_draw(); //< Draw the debug info

        UIContext  *_context    = nullptr; //< Pointer to UIContext
        Widget     *_parent     = nullptr; //< Parent widget
        Widget     *_root       = nullptr; //< Cache of root
        Ref<Style>  _style      = nullptr; //< Pointer to style
        WindowFlags _flags      = WindowFlags::Resizable; //< Window flags
        WidgetAttrs _attrs      = WidgetAttrs::PaintChildren; //< Widget attributrs
        FocusPolicy _focus      = FocusPolicy::Mouse; //< Focus policy
        SizePolicy  _size       = SizePolicy::Expanding; //< Size policy
        Palette     _palette    = {}; //< Palette of widget     
        std::list<Widget *>           _children; //< Child widgets
        std::list<Widget *>::iterator _in_child_iter = {}; //< Child iterator

        Size        _maximum_size = {INT_MAX, INT_MAX}; //< Maximum size
        Size        _minimum_size = {0, 0}; //< Minimum size
        Rect        _rect = {0, 0, 0, 0}; //< Rectangle
        Font        _font    = {}; //< Font
        Cursor      _cursor  = {SystemCursor::Arrow}; //< Cursor
        float       _opacity = 1.0f; //< Opacity

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
inline void Widget::set_fixed_size(Size s) {
    set_fixed_size(s.w, s.h);
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