#pragma once

#include <Btk/detail/keyboard.hpp>
#include <Btk/string.hpp>
#include <Btk/rect.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

// Enum for Mouse Button
enum class MouseButton : uint32_t {
    Left,
    Middle,
    Right,
    X1,
    X2
};

/**
 * @brief Event Base class
 * 
 */
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
            DragBegin   = WidgetEvent + 22, //< Drag begin
            DragMotion  = WidgetEvent + 23, //< Dragging
            DragEnd     = WidgetEvent + 24, //< Drag end
            DropBegin   = WidgetEvent + 25, //< Drop begin
            DropText    = WidgetEvent + 26, //< Drop
            DropFile    = WidgetEvent + 27, //< Drop file
            DropEnd     = WidgetEvent + 28, //< Drop end
            StyleChanged          = WidgetEvent + 29, //< Style changed
            FontChanged           = WidgetEvent + 30, //< Font changed
            PaletteChanged        = WidgetEvent + 31, // < Palette changed
            LayoutRequest         = WidgetEvent + 32, // < Layout changed
            ChildRectangleChanged = WidgetEvent + 33, //< Child's rectangle changed 

            WidgetEnd ,

            Call      , //< EventLoop will call it
            Timer     , //< The timer timeout

            User        = 10086, //< User defined event
            UserMax     = 65535, //< User defined event max
        };

        /**
         * @brief Construct a new empty Event object
         * 
         */
        Event()          : Event(None) {}
        /**
         * @brief Construct a new Event object with type
         * 
         * @param type 
         */
        Event(Type type) : _type(type) {}
        Event(const Event &e) = default;
        Event(Event &&e)      = default;
        ~Event()              = default;

        /**
         * @brief Set the timestamp object
         * 
         * @param time 
         */
        void set_timestamp(uint32_t time) noexcept {
            _timestamp = time;
        }
        /**
         * @brief Set the type object
         * 
         * @param t 
         */
        void set_type(Type t) noexcept {
            _type = t;
        }

        /**
         * @brief Accept the event
         * 
         */
        void accept() noexcept {
            _accepted = true;
        }
        /**
         * @brief Reject the event
         * 
         */
        void reject() noexcept {
            _accepted = false;
        }

        /**
         * @brief Check the event is accepted
         * 
         * @return true 
         * @return false 
         */
        bool is_accepted() const noexcept {
            return _accepted;
        }
        /**
         * @brief Check the event is rejected
         * 
         * @return true 
         * @return false 
         */
        bool is_rejected() const noexcept {
            return !_accepted;
        }
        /**
         * @brief Check the event is widget event
         * 
         * @return true 
         * @return false 
         */
        bool is_widget_event() const noexcept {
            return _type > WidgetBegin && _type < WidgetEnd;
        }

        /**
         * @brief Get the type of the event
         * 
         * @return Type 
         */
        Type        type() const noexcept {
            return _type;
        }
        /**
         * @brief Get the timestamp of the event
         * 
         * @return timestamp_t 
         */
        timestamp_t timestamp() const noexcept {
            return _timestamp;
        }

        // Cast to another event type
        template <typename T>
        T &as() noexcept {
            return *static_cast<T*>(this);
        }
        template <typename T>
        const T &as() const noexcept {
            return *static_cast<const T*>(this);
        }
    private:
        Type        _type      = None;
        uint8_t     _accepted  = false;
        timestamp_t _timestamp = 0;
};

using EventType = Event::Type;

/**
 * @brief Widget Event Base
 * 
 */
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
/**
 * @brief Resize Event
 * 
 */
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
/**
 * @brief Move Event
 * 
 */
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
/**
 * @brief Mouse Motion Event
 * 
 */
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

        Point position() const {
            return Point(_x, _y);
        }

        void set_rel(int x, int y) {
            _xrel = x;
            _yrel = y;
        }
    private:
        int _x = -1;
        int _y = -1;

        // Which value shouble be used for invalid rel values
        int _xrel = 0;
        int _yrel = 0;
};
/**
 * @brief Mouse Button Press or Release
 * 
 */
class MouseEvent : public WidgetEvent {
    public:
        MouseEvent(Type t, int x, int y, uint8_t clicks) : 
            WidgetEvent(t), _x(x), _y(y), _clicks(clicks) {}
        
        int x() const {
            return _x;
        }
        int y() const {
            return _y;
        }
        uint8_t clicks() const {
            return _clicks;
        }
        Point position() const {
            return Point(_x, _y);
        }
        MouseButton button() const {
            return _button;
        }

        void set_button(MouseButton b) {
            _button = b;
        }
        void set_position(int x, int y) {
            _x = x;
            _y = y;
        }
    private:
        int _x; //< Mouse x position
        int _y; //< Mouse y position
        uint8_t _clicks; //< Number of clicks
        MouseButton _button; //< Mouse button
};
/**
 * @brief Mouse Wheel Motion
 * 
 */
class WheelEvent : public WidgetEvent {
    public:
        WheelEvent(int x, int y) 
            : WidgetEvent(MouseWheel), _x(x), _y(y) {}

        int x() const {
            return _x;
        }
        int y() const {
            return _y;
        }
    private:
        int _x; //< Wheel in x
        int _y;
};
/**
 * @brief Tell you, you should paint yourself
 * 
 */
class PaintEvent : public WidgetEvent {
    public:
        PaintEvent() : WidgetEvent(Paint) {}
};
/**
 * @brief Close Event
 * 
 */
class CloseEvent : public WidgetEvent {
    public:
        CloseEvent() : WidgetEvent(Close) {}
};
/**
 * @brief Mouse Begin Drag or Dragging or End Drag
 * 
 */
class DragEvent  : public WidgetEvent {
    public:
        DragEvent(Type t, int x, int y) : WidgetEvent(t), _x(x), _y(y) {}

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

        Point position() const {
            return Point(_x, _y);
        }
    private:
        int _x = -1;
        int _y = -1;
        int _xrel = 0;
        int _yrel = 0;
};
/**
 * @brief Keyboard press or release
 * 
 */
class KeyEvent : public WidgetEvent {
    public:
        KeyEvent(Type t, Key key, Modifier modifiers) : WidgetEvent(t), _key(key), _modifiers(modifiers) {}
        Key      key() const {
            return _key;
        }
        Modifier modifiers() const {
            return _modifiers;
        }
        uint8_t  repeat() const {
            return _repeat;
        }

        void     set_repeat(uint8_t repeat) {
            _repeat = repeat;
        }
    private:
        Key      _key;
        Modifier _modifiers;
        uint8_t  _repeat = 0;
};
/**
 * @brief IME Text 
 * 
 */
class TextInputEvent : public WidgetEvent {
    public:
        TextInputEvent(u8string_view t) : WidgetEvent(TextInput), _text(t) {}

        u8string_view text() const {
            return _text;
        }
    private:
        u8string _text;
};
/**
 * @brief Focus Gain or Focus Lost
 * 
 */
class FocusEvent : public WidgetEvent {
    public:
        FocusEvent(Type type) : WidgetEvent(type) {}
};
/**
 * @brief DropEvent
 * 
 */
class DropEvent : public WidgetEvent {
    public:
        DropEvent() = default;
        DropEvent(Event::Type t) : WidgetEvent(t) {}

        // Set the drop data
        void set_text(u8string_view us) {
            _text = us;
        }
        void set_position(int x, int y) {
            _x = x;
            _y = y;
        }

        int           x() const {
            return _x;
        }
        int           y() const {
            return _y;
        }
        Point         position() const {
            return Point(_x, _y);
        }
        u8string_view text() const {
            return _text;
        }
    private:
        u8string _text;
        int _x;
        int _y;
};
/**
 * @brief Widget something changed
 * 
 */
class ChangeEvent   : public WidgetEvent {
    public:
        using WidgetEvent::WidgetEvent;
};

// Event In child changed, removed or added
class ChildEvent    : public ChangeEvent {
    public:
        ChildEvent(EventType type, Widget *child) : ChangeEvent(type), _child(child) {}

        bool is_added() const {
            return type() == ChildAdded;
        }
        bool is_removed() const {
            return type() == ChildRemoved;
        }
        Widget *child() const  {
            return _child;
        }
    private:
        Widget *_child = nullptr;
};

class TimerEvent : public Event {
    public:
        TimerEvent(Object *obj, timerid_t id) : Event(Timer), _object(obj), _timerid(id) {}
        TimerEvent(const TimerEvent &e) = default;

        Object   *object() const {
            return _object;
        }
        timerid_t timerid() const {
            return _timerid;
        }
    private:
        Object   *_object;
        timerid_t _timerid;
};


// Another Event
class QuitEvent : public Event {
    public:
        QuitEvent()      : Event(Quit) {}
        QuitEvent(int c) : Event(Quit), _code(c) {}

        int code() const {
            return _code;
        }
    private:
        int _code = EXIT_SUCCESS;
};

class CallEvent : public Event {
    public:
        CallEvent() : Event(Call) {}

        void call() {
            _func(_user);
        }

        void set_func(void (*fn)(void *)) {
            _func = fn;
        }
        void set_user(void *user) {
            _user = user;
        }
    private:
        void (*_func)(void *user) = nullptr;
        void  *_user              = nullptr;
};

/**
 * @brief Register your event type
 * 
 * @return EventType 
 */
BTKAPI EventType RegisterEvent();

BTK_NS_END