#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/painter.hpp>
#include <Btk/style.hpp>
#include <thread> //< For std::this_thread::yield()
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#endif

BTK_NS_BEGIN

static UIContext  *ui_context = nullptr; //< Global UI context
static EventType   ui_event   = EventType::User; //< Current Registered event

void SetUIContext(UIContext *context) {
    ui_context = context;
}
UIContext *GetUIContext() {
    return ui_context;
}

UIContext::UIContext(GraphicsDriver *driv) {
    assert(!GetUIContext());

    driver = driv;
    PaletteBreeze(&palette);
    StyleBreeze(&style);
    SetUIContext(this);

#if !defined(NDEBUG)
    puts(palette.to_string().c_str());
#endif

    thread_id = std::this_thread::get_id(); 
}
UIContext::UIContext() {

#if defined(BTK_DRIVER)
    driver = BTK_DRIVER.create();
#else
    abort();
#endif
    PaletteBreeze(&palette);
    StyleBreeze(&style);
    SetUIContext(this);

#if !defined(NDEBUG)
    puts(palette.to_string().c_str());
#endif

    thread_id = std::this_thread::get_id(); 
}
UIContext::~UIContext() {
    for(auto w : widgets) {
        delete w;
    }
    delete driver;

    if (GetUIContext() == this) {
        SetUIContext(nullptr);
    }
}
bool UIContext::run() {
    EventLoop loop(this);
    return loop.run();
}

EventQueue::EventQueue() {

}
EventQueue::~EventQueue() {

}
void EventQueue::push(Event *event) {
    spin.lock();
    queue.push_back(event);
    spin.unlock();
}
void EventQueue::pop() {
    spin.lock();
    queue.pop_front();
    spin.unlock();
}
Event *EventQueue::peek() const {
    spin.lock();
    Event *event = queue.front();
    spin.unlock();
    return event;
}
bool  EventQueue::poll(Event **event) {
    spin.lock();
    Event *ev = nullptr;
    if (!queue.empty()) {
        ev = queue.front();
        queue.pop_front();
    }
    *event = ev;
    spin.unlock();
    return *event;
}

bool EventLoop::run() {
    auto &driver = ctxt->driver;
    auto &queue  = ctxt->queue; 
    Event *event;
    while (running) {
        driver->pump_events(ctxt);

        while(queue.poll(&event)) {
            dispatch(event);
            delete event;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}
void EventLoop::dispatch(Event *event) {
    switch (event->type()) {
        case Event::Quit : {
            running = false;
            break;
        }
        case Event::Timer : {
            auto t = static_cast<TimerEvent*>(event);
            t->object()->handle(*event);
            break;
        }
        case Event::Call : {
            auto c = static_cast<CallEvent*>(event);
            c->call();
            break;
        }
        default : {
            // Widget event
            if (event->is_widget_event()) {
                auto w = static_cast<WidgetEvent*>(event)->widget();
                if (w == nullptr) {
                    BTK_LOG("EventLoop::dispatch: widget is null\n");
                    break;
                }
                // auto iter = ctxt->widgets.find(w);
                // if (iter == ctxt->widgets.end()) {
                //     printf("EventLoop::dispatch: widget not found\n");
                //     break;
                // }

                w->handle(*event);
                break;
            }

            BTK_LOG("EventLoop::dispatch: unknown event type\n");
            break;
        }
    }
}

timestamp_t GetTicks() {
#if   defined(_WIN32)
    // Native here
    if constexpr (sizeof(timestamp_t) >= sizeof(ULONGLONG)) {
        return GetTickCount64();
    }
    else {
        return GetTickCount();
    }
#else
    // Use fallback
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
#endif
}

EventType RegisterEvent() {
    auto cur = ui_event;
    if (cur != EventType::UserMax) {
        ui_event = EventType(uint32_t(ui_event) + 1);
    }
    return cur;
}

BTK_NS_END