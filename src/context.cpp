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

// Style
void StyleBreeze(Style *style) {
    style->background = Color(252, 252, 252);
    style->window     = Color(239, 240, 241);
    // style->button     = Color(238, 240, 241);
    style->border     = Color(188, 190, 191);
    style->highlight  = Color(61 , 174, 233);
    style->selection  = Color(61 , 174, 233);

    style->text           = Color(0, 0, 0);
    style->highlight_text = Color(255, 255, 255);

    // Button default size
    style->button_width = 88;
    style->button_height = 32;
    // Button radius
    style->button_radius = 0;

    style->radio_button_circle_pad = 4;
    style->radio_button_circle_inner_pad = 4;
    style->radio_button_circle_r = 8;
    style->radio_button_text_pad = 4;

    // Progress bar
    style->progressbar_width = 100;
    style->progressbar_height = 20;

    // Slider
    style->slider_height = 20;
    style->slider_width  = 20;
    // style->slider_bar_height = 10;
    // style->slider_bar_width  = 10;

    style->margin = 2.0f;

    // Set font family and size
// #if defined(_WIN32)
#if 0
    // Query current system font
    LOGFONTW lf;
    GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    auto nameu8 = u8string::from(lf.lfFaceName);

    style->font = Font(nameu8, std::abs(lf.lfHeight));
#else
    style->font = Font("", 12);
#endif
    
    // Icon
    style->icon_width = 16;
    style->icon_height = 16;

    // Window default
    style->window_height = 100;
    style->window_width  = 100;

    // Shadow
    style->shadow_radius = 4;
    style->shadow_offset_x = 2;
    style->shadow_offset_y = 2;

    LinearGradient linear;
    linear.add_stop(0, Color::Gray);
    linear.add_stop(1, Color::Transparent);

    style->shadow = linear;

}

EventType RegisterEvent() {
    auto cur = ui_event;
    if (cur != EventType::UserMax) {
        ui_event = EventType(uint32_t(ui_event) + 1);
    }
    return cur;
}

BTK_NS_END