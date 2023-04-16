#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/context.hpp>
#include <Btk/painter.hpp>
#include <Btk/event.hpp>
#include <Btk/style.hpp>
#include <thread> //< For std::this_thread::yield()
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#endif

// From platform module
extern "C" void __BtkPlatform_Init();
// From widgets module
extern "C" void __BtkWidgets_Init();

BTK_NS_BEGIN

thread_local
static EventDispatcher *th_dispatcher = nullptr; //< Thread event dispatcher
static GraphicsDriverInfo *ui_driver = nullptr; //< Default graphics driver creation information
static UIContext  *ui_context = nullptr; //< Global UI context
static EventType   ui_event   = EventType::User; //< Current Registered event

auto SetUIContext(UIContext *context) -> void {
    ui_context = context;
}
auto GetUIContext() -> UIContext *{
    return ui_context;
}
auto GetDispatcher() -> EventDispatcher * {
    return th_dispatcher;
}
auto SetDispatcher(EventDispatcher *d) -> void {
    th_dispatcher = d;
}

UIContext::UIContext(GraphicsDriver *driv) {
    initialize(driv);
}
UIContext::UIContext() {
    BTK_ONCE(__BtkPlatform_Init());
    BTK_ONCE(__BtkWidgets_Init());

    initialize(CreateDriver());
}
UIContext::~UIContext() {
    delete _driver;

    if (GetUIContext() == this) {
        SetUIContext(nullptr);
    }
}
int  UIContext::run() {
    if (_dispatcher == nullptr) {
        return EXIT_FAILURE;
    }
    EventLoop loop(_dispatcher);
    return loop.run();
}
void UIContext::initialize(GraphicsDriver *driv) {
    assert(!GetUIContext());
    assert(driv);

    _palette    = PaletteBreeze();
    _style      = CreateStyle();
    _dispatcher = GetDispatcher(); 
    _driver     = driv;
    SetUIContext(this);

    thread_id = std::this_thread::get_id(); 

    if (!_style) {
        abort();
    }
}
auto UIContext::deskop_service() const -> DesktopService * {
    return _driver->service_of<DesktopService>();
}
auto UIContext::set_clipboard_text(u8string_view txt) -> void {
    return _driver->clipboard_set(txt);
}
auto UIContext::clipboard_text() -> u8string {
    return _driver->clipboard_get();
}

// EventQueue::EventQueue() {

// }
// EventQueue::~EventQueue() {

// }
// void EventQueue::push(Event *event) {
//     spin.lock();
//     queue.push_back(event);
//     spin.unlock();
// }
// void EventQueue::pop() {
//     spin.lock();
//     queue.pop_front();
//     spin.unlock();
// }
// Event *EventQueue::peek() const {
//     spin.lock();
//     Event *event = queue.front();
//     spin.unlock();
//     return event;
// }
// bool  EventQueue::poll(Event **event) {
//     spin.lock();
//     Event *ev = nullptr;
//     if (!queue.empty()) {
//         ev = queue.front();
//         queue.pop_front();
//     }
//     *event = ev;
//     spin.unlock();
//     return *event;
// }

void EventLoop::stop() {
    return dispatcher->interrupt();
}
int  EventLoop::run() {
    return dispatcher->run();
}

bool EventDispatcher::dispatch(Event *event) {
    bool ret = true;
    switch (event->type()) {
        case Event::Quit : {
            interrupt();
            break;
        }
        case Event::Timer : {
            auto t = static_cast<TimerEvent*>(event);
            ret = t->object()->handle(*event);
            break;
        }
        case Event::Call : {
            auto c = static_cast<CallEvent*>(event);
            c->call();
            break;
        }
        case Event::ClipbordUpdate : {
            BTK_LOG("[EventDispatcher] ClipbordUpdate\n")
            GetUIContext()->signal_clipboard_update().emit();
            break;
        }
        default : {
            // Widget event
            if (event->is_widget_event()) {
                auto w = static_cast<WidgetEvent*>(event)->widget();
                if (w == nullptr) {
                    BTK_LOG("[EventDispatcher::dispatch] widget is null\n");
                    break;
                }
                // auto iter = ctxt->widgets.find(w);
                // if (iter == ctxt->widgets.end()) {
                //     printf("EventLoop::dispatch: widget not found\n");
                //     break;
                // }

                ret = w->handle(*event);
                break;
            }
            ret = false;
            BTK_LOG("[EventDispatcher::dispatch] unknown event type\n");
            break;
        }
    }
    return ret;
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

auto RegisterDriver(GraphicsDriverInfo &info) -> void{
    ui_driver = &info;
}
auto CreateDriver() -> GraphicsDriver * {
    assert(ui_driver);
    return ui_driver->create();
}

BTK_NS_END