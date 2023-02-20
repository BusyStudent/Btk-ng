#pragma once

#include <Btk/detail/threading.hpp>
#include <Btk/string.hpp>
#include <Btk/object.hpp>
#include <Btk/pixels.hpp>
#include <Btk/style.hpp>
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
        void initialize(GraphicsDriver *driver);

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