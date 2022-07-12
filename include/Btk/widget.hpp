#pragma once

#include <Btk/painter.hpp>
#include <Btk/object.hpp>
#include <Btk/string.hpp>
#include <Btk/style.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#include <list>

#define BTK_EXPOSE_SIGNAL(name) \
    auto &signal##name() { \
        return name;\
    }

BTK_NS_BEGIN

class KeyEvent;
class MouseEvent;
class PaintEvent;
class MotionEvent;

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
};
// Enum for widget.

BTK_FLAGS_OPERATOR(WindowFlags, uint32_t);

// Event filter, return true to discard event
using EventFilter = bool (*)(Widget *, Event &, void *);

class EventFilterNode {
    public:
        EventFilter filter;
        void *userdata;
        EventFilterNode *next;
};
/**
 * @brief Widget base class
 * 
 */
class BTKAPI Widget : public Object {
    public:
        Widget();
        Widget(Widget *parent);
        ~Widget();

        bool handle(Event &event) override;

        // Configure
        void show();
        void hide();
        void repaint();

        void set_visible(bool visible);
        void set_rect(int x, int y, int w, int h);
        void move(int x, int y);

        void resize(int width, int height);
        void resize(const Point &p) {
            resize(p.x, p.y);
        }

        // Querys
        Rect    rect() const;
        bool    visible() const;
        Widget *parent() const;
        Widget *window() const;
        Widget *root() const;
        Style  *style() const;
        bool    is_window() const;
        auto    font() const -> const Font &;

        int     width() const {
            return rect().w;
        }
        int     height() const {
            return rect().h;
        }

        // Size Hints
        virtual Size size_hint() const;

        // Event handlers
        virtual bool key_press    (KeyEvent &) { return false; }
        virtual bool key_release  (KeyEvent &) { return false; }

        virtual bool mouse_press  (MouseEvent &) { return false; }
        virtual bool mouse_release(MouseEvent &) { return false; }
        virtual bool mouse_enter  (MotionEvent &) { return false; }
        virtual bool mouse_leave  (MotionEvent &) { return false; }
        virtual bool mouse_motion (MotionEvent &) { return false; }

        virtual bool paint_event(PaintEvent &) { return false; }

        // Get window associated with widget
        window_t   winhandle() const;
        driver_t   driver() const;
        Painter   &painter()   const;

        // Child widgets
        void       add_child(Widget *child);
        bool       has_child(Widget *child) const;
        void       remove_child(Widget *child);
        Widget    *child_at(int x, int y) const;

        // Configure window
        void set_window_title(u8string_view title);
        void set_window_size(int width, int height);
        void set_window_position(int x, int y);
        void capture_mouse(bool capture);

        // EventFilter
        void add_event_filter(EventFilter filter, pointer_t data);
        void del_event_filter(EventFilter filter, pointer_t data);
    private:
        EventFilterNode *_filters = nullptr   ;//< Event filter list
        UIContext  *_context    = nullptr;//< Pointer to UIContext
        Widget     *_parent    = nullptr;//< Parent widget
        Style      *_style      = nullptr;//< Pointer to style
        WindowFlags _flags      = WindowFlags::Resizable;//< Window flags
        std::list<Widget *> _children;//< Child widgets
        std::list<Widget *>::iterator _in_child_iter = {};//< Child iterator

        Rect        _rect = {0, 0, 0, 0};//< Rectangle
        Font        _font    = {};//< Font

        window_t    _win     = {};//< Window handle
        Painter     _painter = {};//< Painter
        uint8_t     _painter_inited  = false;//< Is painter inited ?

        // Attributes
        uint8_t     _accept_drop = false;//< Is drop accepted ?
        uint8_t     _visible     = true;//< Visibility
        uint8_t     _enabled     = true;//< Enabled
        uint8_t     _drag_reject = false;//< Is drag rejected ?

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

class BTKAPI CheckButton : public AbstractButton {
    public:
        CheckButton();
        ~CheckButton();

        void set_checked(bool checked);
        bool is_checked() const;
};

class BTKAPI RadioButton : public AbstractButton {
    public:
        RadioButton();
        ~RadioButton();

        void set_checked(bool checked);
        bool is_checked() const;
};
class BTKAPI Button      : public AbstractButton {
    public:
        Button(Widget *parent = nullptr);
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


// Frame 
class BTKAPI Frame : public Widget {

};

// TextEdit

class BTKAPI LineEdit : public Widget {

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

        Direction _direction = LeftToRight;
        bool      _text_visible = false;

        Signal<void()> _value_changed;
        Signal<void()> _range_changed;
};


// Layout Tools

class LayoutItem {

};
class Layout : public Trackable {
    public:
        void attach(Widget *widget);
};

BTK_NS_END