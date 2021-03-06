#pragma once

#include <Btk/detail/threading.hpp>
#include <Btk/painter.hpp>
#include <Btk/string.hpp>
#include <Btk/object.hpp>
#include <Btk/widget.hpp>
#include <Btk/pixels.hpp>
#include <Btk/event.hpp>
#include <unordered_set>
#include <atomic>
#include <deque>

#define BTK_MAKE_DEVINFO(type, name, text) \
    GraphicsDriverInfo name = { \
        [] () -> GraphicsDriver * { \
            return new type; \
        },  \
        text, \
    };

// All Abstract Interfaces methods name by xxx_op()


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
        };


        // Window management
        virtual window_t window_create(const char_t * title, int width, int height,WindowFlags flags) = 0;

        // Clipboard
        virtual void     clipboard_set(const char_t * text) = 0;
        virtual u8string clipboard_get() = 0;

        // Graphics context
        // 
        // virtual gcontext_t   gc_create(int from,...) = 0;
        // virtual vgcontext_t vgc_create(int from,...) = 0;
        // virtual g3context_t gc3_create(int from,...) = 0;

        // virtual void       gc_draw_line(gcontext_t gc, float x1, float y1, float x2, float y2) = 0;
        // virtual void       gc_draw_rect(gcontext_t gc, float x, float y, float width, float height) = 0;
        // virtual void       gc_fill_rect(gcontext_t gc, float x, float y, float width, float height) = 0;

        // virtual void       gc_begin() = 0;
        // virtual void       gc_end() = 0;
        // Event collect

        // Font
        // virtual font_t     font_create(const char_t * name) = 0;
        // virtual Stringlist font_list() = 0;


        // Timer
        virtual timerid_t  timer_add(Object *object, timertype_t type, uint32_t ms) = 0;
        virtual bool       timer_del(Object *object, timerid_t id) = 0;

        virtual void       pump_events(UIContext *) = 0;
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
 * @brief 3D Context like OpenGL
 * 
 */
class Graphics3DContext : public Any {
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
class GLContext : public Graphics3DContext {
    public:
        virtual bool      initialize(const GLFormat &) = 0;
        virtual bool      set_sync(bool v) = 0;
        virtual bool      set_viewport(const Rect &rect) = 0;
        virtual Size      get_drawable_size() = 0;
        virtual pointer_t get_proc_address(const char_t *name) = 0;
};

class AbstractWindow : public Any {
    public:
        enum Query : int {
            NativeHandle,
            Hwnd,
            Hdc,
            XDisplay,
            XWindow,
        };
        enum Attr : int {
            AcceptDrop,
            Fullscreen,
            Maximized,
            Minimized,
            Resizable,
        };

        virtual Size       size() const = 0;
        virtual Point      position() const = 0;
        virtual void       raise() = 0;
        // virtual void       grab() = 0;
        virtual void       close() = 0; //< Send close event to window
        virtual void       repaint() = 0;
        virtual void       show(bool show_flag) = 0;
        virtual void       move  (int x, int y) = 0;
        virtual void       resize(int width, int height) = 0;
        virtual void       set_title(const char_t * title) = 0;
        virtual void       set_icon(const PixBuffer &buffer) = 0;
        virtual void       set_textinput_rect(const Rect &rect) = 0;
        virtual void       capture_mouse(bool capture) = 0;
        virtual void       start_textinput(bool start) = 0;
        // virtual bool       set_attribute(int attr, ...) = 0;

        // virtual pointer_t native_handle() = 0;
        virtual widget_t   bind_widget(widget_t widget) = 0;
        // virtual gcontext_t gc_create() = 0;
        virtual Painter    painter_create() = 0;
};


#if 1

extern GraphicsDriverInfo Win32DriverInfo;
extern GraphicsDriverInfo SDLDriverInfo;

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

enum class EventWalk {
    Continue = 0,
    Stop     = 1,
    Drop     = 1 << 1,
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
        EventLoop(UIContext *ctxt) : ctxt(ctxt) {}
        ~EventLoop() = default;

        bool run();
        void stop() {
            running = false;
        }
    private:
        void dispatch(Event *);

        UIContext *ctxt = nullptr;
        bool running = true;
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
            queue.push(new T(event));
        }
        template <typename T>
        void walk_event(T &&callback) {
            queue.walk(std::forward<T>(callback));
        }

        //Expose Loop

        bool run();

        //Expose Driver

        GraphicsDriver *graphics_driver() {
            return driver;
        }
        EventQueue     &event_queue() {
            return queue;
        }
        timerid_t timer_add(Object *obj, timertype_t t, uint32_t ms) {
            return driver->timer_add(obj, t, ms);
        }
        bool      timer_del(Object *obj,timerid_t id) {
            return driver->timer_del(obj, id);
        }
    private:
        PainterInitializer painter_init;
        GraphicsDriver *driver = nullptr;
        EventQueue queue;//< For collecting events
        std::unordered_set<Widget *> widgets;
        Style style;
    friend class WidgetImpl;
    friend class Widget;
    friend class EventLoop;
};

BTKAPI UIContext *GetUIContext();
BTKAPI void       SetUIContext(UIContext *context);

inline EventLoop::EventLoop() : EventLoop(GetUIContext()) {}

BTK_NS_END