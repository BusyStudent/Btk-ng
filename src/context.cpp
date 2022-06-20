#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/style.hpp>
#include <thread> //< For std::this_thread::yield()
#include <chrono>

BTK_NS_BEGIN

static thread_local UIContext *ui_context = nullptr;

void SetUIContext(UIContext *context) {
    ui_context = context;
}
UIContext *GetUIContext() {
    return ui_context;
}

UIContext::UIContext(GraphicsDriver *driv) {
    driver = driv;
    StyleBreeze(&style);
    SetUIContext(this);
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
        case Event::WidgetBegin ... Event::WidgetEnd : {
            auto w = static_cast<WidgetEvent*>(event)->widget();
            if (w == nullptr) {
                printf("EventLoop::dispatch: widget is null\n");
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
            printf("EventLoop::dispatch: unknown event type\n");
            break;
        }
    }
}

timestamp_t GetTicks() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Style
void StyleBreeze(Style *style) {
    style->background = Color(252, 252, 252);
    style->window     = Color(239, 240, 241);
    style->button     = Color(238, 240, 241);
    style->border     = Color(188, 190, 191);
    style->highlight  = Color(61 , 174, 233);
    style->selection  = Color(61 , 174, 233);

    style->text           = Color(0, 0, 0);
    style->highlight_text = Color(255, 255, 255);

    // Button default size
    style->button_width = 88;
    style->button_height = 32;
}

BTK_NS_END