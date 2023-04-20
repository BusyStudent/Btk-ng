#pragma once

#include <Btk/string.hpp>
#include <Btk/object.hpp>
#include <Btk/pixels.hpp>
#include <Btk/style.hpp>
#include <thread>

#define BTK_MAKE_DEVINFO(type, name, text) \
    GraphicsDriverInfo name = { \
        [] () -> GraphicsDriver * { \
            return new type; \
        },  \
        text, \
    };

// All Abstract Interfaces methods to control instance name by xxx_op()


BTK_NS_BEGIN

// class BTKAPI EventQueue {
//     public:
    
//         EventQueue();
//         ~EventQueue();

//         void push(Event * event);
//         void pop();

//         bool empty() const;
//         bool poll(Event **event);

//         Event *peek() const;

//         template <typename T>
//         void walk(T &&callback) {
//             for (auto iter = queue.begin(); iter != queue.end();) {
//                 auto wlk = callback(**iter);
//                 if ((wlk & EventWalk::Drop) == EventWalk::Drop) {
//                     iter = queue.erase(iter);
//                 }
//                 else if ((wlk & EventWalk::Stop) == EventWalk::Stop) {
//                     break;
//                 }
//                 else {
//                     ++iter;
//                 }
//             }
//         }
//     private:
//         //TODO : Use a lock free queue
//         //TODO : Use Holder to prevent heap allocation
//         mutable SpinLock spin;
//         std::deque<Event *> queue;
// };

class BTKAPI EventLoop : public Trackable {
    public:
        EventLoop();
        EventLoop(EventDispatcher *dispatcher) : dispatcher(dispatcher) {}
        ~EventLoop() = default;

        int  run();
        void stop();
    private:
        EventDispatcher *dispatcher;
};

class BTKAPI UIContext : public Trackable {
    public:
        UIContext();
        UIContext(GraphicsDriver *driver);
        UIContext(const UIContext &) = delete;
        ~UIContext();

        // Expose Loop
        int  run();

        // Expose Driver
        auto driver()     -> GraphicsDriver * {
            return _driver;
        }
        auto dispatcher() -> EventDispatcher *{
            return _dispatcher;
        }
        auto style()      -> Style          *{
            return _style.get();
        }
        auto palette()    -> const Palette & {
            return _palette;
        }
        auto font()       -> const Font & {
            return _style->font;
        }

        // Configure
        auto set_font(const Font &f)            -> void {
            _style->font = f;
        }
        auto set_style(const Ref<Style> &s)      -> void {
            _style = s;
        }
        auto set_palette(const Palette &p)       -> void {
            _palette = p;
        }
        auto set_dispatcher(EventDispatcher *d) -> void {
            _dispatcher = d;
        }

        // Query
        auto ui_thread_id() const -> std::thread::id {
            return _thread_id;
        }

        // Service
        auto deskop_service() const -> DesktopService *;

        // Signal
        auto signal_clipboard_update() -> Signal<void()> & {
            return _clipboard_update;
        }

        // Clipboard
        auto set_clipboard_text(u8string_view text) -> void;
        auto clipboard_text()                       -> u8string;

        // Screen
        auto num_of_screen()                        -> int;
    private:
        void initialize(GraphicsDriver *driver);

        PainterInitializer painter_init;
        EventDispatcher *_dispatcher = nullptr;
        GraphicsDriver  *_driver = nullptr;
        std::thread::id _thread_id;
        Palette  _palette;
        Ref<Style> _style;

        Signal<void()> _clipboard_update;
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