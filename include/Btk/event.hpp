#pragma once

#include <Btk/detail/keyboard.hpp>
#include <Btk/string.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class Event {
    public:
        enum Type : uint32_t {
            None = 0,
            Quit = 1,

            // Widget events
            WidgetEvent = 2,
            WidgetBegin = WidgetEvent,

            Moved       = WidgetEvent + 1,
            Resized     = WidgetEvent + 2,
            Show        = WidgetEvent + 3,
            Hide        = WidgetEvent + 4,
            FocusGained = WidgetEvent + 5,
            FocusLost   = WidgetEvent + 6,
            MouseEnter  = WidgetEvent + 7,
            MouseLeave  = WidgetEvent + 8,
            MouseMotion = WidgetEvent + 9,
            MousePress  = WidgetEvent + 10,
            MouseRelease= WidgetEvent + 11,
            MouseWheel  = WidgetEvent + 12,
            KeyPress    = WidgetEvent + 13,
            KeyRelease  = WidgetEvent + 14, //< Key release
            TextInput   = WidgetEvent + 15, //< Keyboard text input
            TextEdit    = WidgetEvent + 16, //< Keyboard text edit
            Close       = WidgetEvent + 17, //< User close widget
            Paint       = WidgetEvent + 18, //< Paint widget right now
            ChildAdded  = WidgetEvent + 19, //< Child added to parent
            ChildRemoved= WidgetEvent + 20, //< Child removed from parent
            Reparent    = WidgetEvent + 21, //< Parent changed
            
            WidgetEnd ,

            Call      , //< EventLoop will call it
            Timer     , //< The timer timeout

        };

        Event()          : Event(None) {}
        Event(Type type) : _type(type) {}
        Event(const Event &e) = default;
        Event(Event &&e)      = default;
        ~Event()              = default;

        void set_timestamp(uint32_t time) noexcept {
            _timestamp = time;
        }
        void set_type(Type t) noexcept {
            _type = t;
        }

        void accept() noexcept {
            _accepted = true;
        }
        void reject() noexcept {
            _accepted = false;
        }

        bool is_accepted() const noexcept {
            return _accepted;
        }
        bool is_rejected() const noexcept {
            return !_accepted;
        }
        bool is_widget_event() const noexcept {
            return _type >= WidgetEvent && _type < Close;
        }

        Type        type() const noexcept {
            return _type;
        }
        timestamp_t timestamp() const noexcept {
            return _timestamp;
        }
    private:
        Type        _type      = None;
        uint8_t     _accepted  = false;
        timestamp_t _timestamp = 0;
};

class WidgetEvent : public Event {
    public:
        using Event::Event;

        void    set_widget(Widget *w) noexcept {
            _widget = w;
        }
        Widget *widget() const noexcept {
            return _widget;
        }
    private:
        Widget *_widget = nullptr;//< Widget the event sent to
};
class ResizeEvent : public WidgetEvent {
    public:
        ResizeEvent() : WidgetEvent(Resized) {}
        ResizeEvent(int width, int height) : WidgetEvent(Resized), _width(width), _height(height) {}

        int width() const {
            return _width;
        }
        int height() const {
            return _height;
        }

        int old_width() const {
            return _old_width;
        }
        int old_height() const {
            return _old_height;
        }

        void set_old_size(int width, int height) {
            _old_width = width;
            _old_height = height;
        }
        void set_new_size(int width, int height) {
            _width = width;
            _height = height;
        }
    private:
        int _width = -1;
        int _height = -1;

        int _old_width = -1;
        int _old_height = -1;
};
class MoveEvent : public WidgetEvent {
    public:
        MoveEvent(int x, int y) : WidgetEvent(Moved), _x(x), _y(y) {}

        int x() const {
            return _x;
        }
        int y() const {
            return _y;
        }
    private:
        int _x = -1;
        int _y = -1;

        int _old_x = -1;
        int _old_y = -1;
};
class MotionEvent : public WidgetEvent {
    public:
        MotionEvent(int x, int y) : WidgetEvent(MouseMotion), _x(x), _y(y) {}

        int x() const {
            return _x;
        }
        int y() const {
            return _y;
        }
        int xrel() const {
            return _xrel;
        }
        int yrel() const {
            return _yrel;
        }

        void set_rel(int x, int y) {
            _xrel = x;
            _yrel = y;
        }
    private:
        int _x = -1;
        int _y = -1;

        int _xrel = -1;
        int _yrel = -1;
};
class MouseEvent : public WidgetEvent {
    public:
        MouseEvent(Type t, int x, int y, uint8_t clicks) : 
            WidgetEvent(t), _x(x), _y(y), _clicks(clicks) {}
        
        int x() const;
        int y() const;
        int button() const;
    private:
        int _x; //< Mouse x position
        int _y; //< Mouse y position
        int _button; //< Mouse button
        uint8_t _clicks; //< Number of clicks
};
class PaintEvent : public WidgetEvent {
    public:
        PaintEvent() : WidgetEvent(Paint) {}
};
class CloseEvent : public WidgetEvent {
    public:
        CloseEvent() : WidgetEvent(Close) {}
};
class KeyEvent : public WidgetEvent {
    public:
        KeyEvent(Type t, Key key, Modifier modifiers) : WidgetEvent(t), _key(key), _modifiers(modifiers) {}
        Key      key() const {
            return _key;
        }
        Modifier modifiers() const {
            return _modifiers;
        }
    private:
        Key      _key;
        Modifier _modifiers;
};
class TextInputEvent : public WidgetEvent {
    private:
        u8string _text;
};
class ReparentEvent : public WidgetEvent {
    private:
        Widget *_old;
        Widget *_new;
};

class TimerEvent : public Event {
    public:
        TimerEvent(Object *obj, uint32_t id) : Event(Timer), _object(obj), _timerid(id) {}
        TimerEvent(const TimerEvent &e) = default;

        Object  *object() const {
            return _object;
        }
        uint32_t timerid() const {
            return _timerid;
        }
    private:
        Object   *_object;
        timerid_t _timerid;
};


// Another Event

class CallEvent : public Event {
    public:
        void call() {
            _func(_user);
        }
    private:
        void (*_func)(void *user);
        void  *_user;
};



union EventCollections {
    Event event = {};
    WidgetEvent widget;
    ResizeEvent resize;
    MouseEvent mouse;
    CallEvent call;
    KeyEvent key;
};

BTK_NS_END