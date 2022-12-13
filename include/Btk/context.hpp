#pragma once

#include <Btk/detail/threading.hpp>
#include <Btk/painter.hpp>
#include <Btk/string.hpp>
#include <Btk/object.hpp>
#include <Btk/widget.hpp>
#include <Btk/pixels.hpp>
#include <Btk/event.hpp>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <deque>

#define BTK_MAKE_DEVINFO(type, name, text) \
    GraphicsDriverInfo name = { \
        [] () -> GraphicsDriver * { \
            return new type; \
        },  \
        text, \
    };

// All Abstract Interfaces methods to control instance name by xxx_op()


BTK_NS_BEGIN

class EventQueue;
class GraphicsDriver;
class GraphicsDriverInfo;
class AbstractWindow;

class GraphicsDriverInfo {
    public:
        GraphicsDriver *(*create)();
        const char       *name;
};

class GraphicsDriver : public Any {
    public:
        enum Feature {
            Reparent, //< Can reparent window
            OpenGL, //< Supports OpenGL
            Vulkan, //< Supports Vulkan
            Metal, //< Supports Metal
            MultiWindow, //< Supports create more than one window
        };
        enum Query {
            SystemDpi,   //< System dpi (*FPoint)
            NumOfScreen, //< Number of screen (*int)
        };


        // Window management
        virtual window_t window_create(const char_t * title, int width, int height, WindowFlags flags) = 0;

        // Clipboard
        virtual void     clipboard_set(const char_t * text) = 0;
        virtual u8string clipboard_get() = 0;

        // Cursor
        virtual cursor_t cursor_create(const PixBuffer &pixbuf, int hot_x, int hot_y) = 0;

        // Dynamic factory
        virtual any_t    instance_create(const char_t *what, ...) = 0;

        // Query
        virtual bool     query_value(int query, ...) = 0;
};

// Provide more details of desktop
class DesktopDriver : public GraphicsDriver {
    public:
        // TODO : 
        virtual pointer_t menu_create() = 0;

        // Extend of clipboard
        virtual void      clipboard_set_image(const PixBuffer &) = 0;
        virtual PixBuffer clipboard_get_image() = 0;
};

/**
 * @brief Graphics Context like OpenGL
 * 
 */
class GraphicsContext : public Any {
    public:
        virtual void      begin() = 0;
        virtual void      end() = 0;
        virtual void      swap_buffers() = 0;
};

class GLFormat {
    public:
        uint8_t red_size     = 8;
        uint8_t green_size   = 8;
        uint8_t blue_size    = 8;
        uint8_t alpha_size   = 8;
        uint8_t depth_size   = 24;
        uint8_t stencil_size = 8;
        uint8_t buffer_count = 1; //< Double buffer or etc.
        //< MSAA Fields
        uint8_t samples = 1;
        uint8_t sample_buffers = 0;
};

/**
 * @brief OpenGL graphics context.
 * 
 */
class GLContext : public GraphicsContext {
    public:
        virtual bool      initialize(const GLFormat &) = 0;
        virtual bool      make_current() = 0;
        virtual bool      set_swap_interval(int v) = 0;
        virtual Size      get_drawable_size() = 0;
        virtual pointer_t get_proc_address(const char_t *name) = 0;
};
class VkContext : public GraphicsContext {
    public:
        virtual pointer_t get_proc_address(const char_t *name) = 0;
};
class FbContext : public GraphicsContext {
    public:
        virtual pointer_t get_pixels_address() = 0;
        virtual PixFormat get_pixels_format() = 0;
        virtual Size      get_size() = 0;
};

class AbstractWindow : public Any {
    public:
        enum ShowFlag : int {
            Hide,
            Show,
            Restore,
            Maximize,
            Minimize,
        };
        enum Value : int {
            NativeHandle,
            Hwnd,        //< Win32 Window Handle (*HWND)
            Hdc,         //< Win32 Driver Context (*HDC)
            XConnection, //< Xcb  Connection  (*xcb_connection_t*)
            XDisplay,    //< Xlib Display     (*Display)
            XWindow,     //< Window           (*Window)
            Dpi,         //< Dpi              (*FPoint)
            MousePosition, //< MousePosition  (*Point)
            MaximumSize, //< args (*Size)
            MinimumSize, //< args (*Size)
        };

        virtual Size       size() const = 0;
        virtual Point      position() const = 0;
        virtual void       raise() = 0;
        // virtual void       grab() = 0;
        virtual void       close() = 0; //< Send close event to window
        virtual void       repaint() = 0;
        virtual void       show(int show_flag) = 0;
        virtual void       move  (int x, int y) = 0;
        virtual void       resize(int width, int height) = 0;
        virtual void       set_title(const char_t * title) = 0;
        virtual void       set_icon(const PixBuffer &buffer) = 0;
        virtual void       set_textinput_rect(const Rect &rect) = 0;
        virtual void       capture_mouse(bool capture) = 0;
        virtual void       start_textinput(bool start) = 0;

        // Flags control
        virtual bool       set_flags(WindowFlags flags) = 0;
        virtual bool       set_value(int conf, ...) = 0;
        virtual bool       query_value(int query, ...) = 0;

        virtual widget_t   bind_widget(widget_t widget) = 0;
        virtual any_t      gc_create(const char_t *type) = 0;
        // [[deprecated("Use Painter::FromWindow()")]]
        // virtual Painter    painter_create() = 0;
};

class AbstractCursor : public Any {
    public:
        // Refcounting manage
        virtual void       ref()   = 0;
        virtual void       unref() = 0;

        virtual void       set() = 0;
};
class AbstractScreen : public Any {
    public:
        virtual Size       pixel_size() = 0;
        virtual FPoint     dpi()        = 0;
};

class AbstractFileDialog : public Any {
    public:
        virtual int        run() = 0;
        virtual bool       initialize(bool save) = 0;
        virtual void       set_dir(u8string_view dir) = 0;
        virtual void       set_title(u8string_view title) = 0;
        virtual void       set_allow_multi(bool v) = 0;
        virtual StringList result() = 0;
};


#if 1

extern GraphicsDriverInfo Win32DriverInfo;
extern GraphicsDriverInfo SDLDriverInfo;
extern GraphicsDriverInfo XcbDriverInfo;

#endif

class EventHolder {
    public:
        EventHolder() = default;
        EventHolder(const EventHolder &h) {
            BTK_ASSERT(h.pevent == nullptr);

            Btk_memcpy(&event_buffer, &h.event_buffer, sizeof(event_buffer));
            destroy = h.destroy;
        }
        EventHolder(EventHolder &&h) {
            if (h.pevent) {
                pevent = h.pevent;
                h.pevent = nullptr;
            }
            else {
                Btk_memcpy(&event_buffer, &h.event_buffer, sizeof(event_buffer));
            }
            destroy = h.destroy;
        }
        ~EventHolder() {
            if (destroy) {
                destroy(event());
            }
            if (pevent) {
                //Cleanup memory
                operator delete(pevent);
            }
        }

        template <typename T>
        EventHolder(T &&event) {
            if constexpr(!std::is_default_constructible_v<T>) {
                destroy = [](Event *e) {
                    static_cast<T *>(e)->~T();
                };
            }
            // Check buffer is large enough
            if constexpr(sizeof(T) > sizeof(event_buffer) ) {
                pevent = new T(std::forward<T>(event));
            }
            else {
                //Construct event at event_buffer
                new (&event_buffer) T(std::forward<T>(event));
            }
        }

        Event *event() {
            if (pevent) {
                return pevent;
            }
            return reinterpret_cast<Event *>(&event_buffer);
        }
    private:
        uint8_t event_buffer[sizeof(EventCollections)];//< For Hold common events
        Event *pevent = nullptr;//< Pointer to the Event if event is bigger than EventCollections
        void (*destroy)(Event *) = nullptr;//< Destroy function for the event
};

using      EventDtor = void (*)(Event *);
enum class EventWalk {
    Continue = 0,
    Stop     = 1,
    Drop     = 1 << 1,
};

/**
 * @brief Interface to access EventQueue
 * 
 */
class EventDispatcher {
    public:
        // Timer
        virtual timerid_t timer_add(Object *object, timertype_t type, uint32_t ms) = 0;
        virtual bool      timer_del(Object *object, timerid_t id) = 0;

        virtual bool      send(Event *event, EventDtor dtor = nullptr) = 0;
        virtual pointer_t alloc(size_t n) = 0;

        virtual void      interrupt() = 0;
        virtual int       run() = 0;

        template <typename T>
        inline  bool      send(T &&event) {
            // Construct event
            using Ty = std::decay_t<T>;
            auto addr = alloc(sizeof(Ty));
            new (addr) Ty(std::forward<T>(event));

            if constexpr (std::is_trivially_destructible_v<Ty>) {
                return send(static_cast<Event*>(addr), nullptr);
            }
            else {
                return send(static_cast<Event*>(addr), [](Event *ev) {
                    static_cast<Ty*>(ev)->~Ty();
                });
            }
        }

        BTKAPI bool      dispatch(Event *);
};

BTK_FLAGS_OPERATOR(EventWalk, int);

class BTKAPI EventQueue {
    public:
    
        EventQueue();
        ~EventQueue();

        void push(Event * event);
        void pop();

        bool empty() const;
        bool poll(Event **event);

        Event *peek() const;

        template <typename T>
        void walk(T &&callback) {
            for (auto iter = queue.begin(); iter != queue.end();) {
                auto wlk = callback(**iter);
                if ((wlk & EventWalk::Drop) == EventWalk::Drop) {
                    iter = queue.erase(iter);
                }
                else if ((wlk & EventWalk::Stop) == EventWalk::Stop) {
                    break;
                }
                else {
                    ++iter;
                }
            }
        }
    private:
        //TODO : Use a lock free queue
        //TODO : Use Holder to prevent heap allocation
        mutable SpinLock spin;
        std::deque<Event *> queue;
};

class BTKAPI EventLoop : public Trackable {
    public:
        EventLoop();
        EventLoop(EventDispatcher *dispatcher) : dispatcher(dispatcher) {}
        ~EventLoop() = default;

        int  run() {
            return dispatcher->run();
        }
        void stop() {
            return dispatcher->interrupt();
        }
    private:
        EventDispatcher *dispatcher;
};

class BTKAPI UIContext : public Trackable {
    public:
        UIContext();
        UIContext(GraphicsDriver *driver);
        UIContext(const UIContext &) = delete;
        ~UIContext();

        // Expose Event

        template <typename T>
        void send_event(const T &event) {
            _dispatcher->send(event);
        }
        // template <typename T>
        // void walk_event(T &&callback) {
        //     queue.walk(std::forward<T>(callback));
        // }

        // Expose Loop
        int  run();

        // Expose Driver
        auto driver()     -> GraphicsDriver * {
            return _driver;
        }
        auto dispatcher() -> EventDispatcher *{
            return _dispatcher;
        }
        auto timer_add(Object *obj, timertype_t t, uint32_t ms) -> timerid_t {
            return _dispatcher->timer_add(obj, t, ms);
        }
        auto timer_del(Object *obj, timerid_t id)               -> bool      {
            return _dispatcher->timer_del(obj, id);
        }

        // Configure
        auto set_font(const Font &f)            -> void {
            style.font = f;
        }
        auto set_dispatcher(EventDispatcher *d) -> void {
            _dispatcher = d;
        }

        // Query
        auto ui_thread_id() const -> std::thread::id {
            return thread_id;
        }
    private:
        PainterInitializer painter_init;
        EventDispatcher *_dispatcher = nullptr;
        GraphicsDriver  *_driver = nullptr;
        std::thread::id thread_id;
        Palette palette;
        Style style;
    friend class Widget;
    friend class EventLoop;
};

BTKAPI auto GetUIContext()                    -> UIContext *;
BTKAPI auto SetUIContext(UIContext *context)  -> void;

BTKAPI auto GetDispatcher()                   -> EventDispatcher *;
BTKAPI auto SetDispatcher(EventDispatcher *)  -> void;

inline auto GetUIDisplatcher()                -> EventDispatcher * {
    return GetUIContext()->dispatcher();
}

inline EventLoop::EventLoop() : EventLoop(GetDispatcher()) {}
inline driver_t   GetDriver() {
    return GetUIContext()->driver();
}

BTK_NS_END