#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/context.hpp>
#include <Btk/event.hpp>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL.h>
#include <unordered_map>

// Import compile platfrom common headers

#if   defined(_WIN32)
#include "common/win32/backtrace.hpp"
#include "common/win32/dialog.hpp"
#include "common/win32/dwmapi.hpp"
#elif defined(__gnu_linux__)
#include "common/x11/backtrace.hpp"
#include "common/x11/dialog.hpp"
#endif


// Remove xlib macro 

#undef KeyPress
#undef KeyRelease
#undef None

// Internal use helper functions

#include "sdl2_trkey.hpp"

BTK_PRIV_BEGIN

using namespace BTK_NAMESPACE;

// Ability of SDL_Renderer

#define SDL_GC_FLOAT  SDL_VERSION_ATLEAST(2, 0, 1)
#define SDL_GC_VERTEX SDL_VERSION_ATLEAST(2, 0, 18)

// Event

#define SDL_TIMER_EVENT   (SDL_LASTEVENT - 10086)
#define SDL_REPAINT_EVENT (SDL_LASTEVENT - 10087)

// Type checker

static_assert(offsetof(SDL_Point, x) == offsetof(Point, x));
static_assert(offsetof(SDL_Point, y) == offsetof(Point, y));

static_assert(offsetof(SDL_Rect, x) == offsetof(Rect, x));
static_assert(offsetof(SDL_Rect, y) == offsetof(Rect, y));
static_assert(offsetof(SDL_Rect, w) == offsetof(Rect, w));
static_assert(offsetof(SDL_Rect, h) == offsetof(Rect, h));

// Capability of SDL_Renderer

#if SDL_GC_FLOAT
static_assert(offsetof(SDL_FPoint, x) == offsetof(FPoint, x));
static_assert(offsetof(SDL_FPoint, y) == offsetof(FPoint, y));

static_assert(offsetof(SDL_FRect, x) == offsetof(FRect, x));
static_assert(offsetof(SDL_FRect, y) == offsetof(FRect, y));
static_assert(offsetof(SDL_FRect, w) == offsetof(FRect, w));
static_assert(offsetof(SDL_FRect, h) == offsetof(FRect, h));
#endif


class SDLDriver ;
class SDLWindow ;

class SDLTimer {
    public:
        SDL_TimerID timer  = 0;
        Object     *object = nullptr;
};

class SDLWindow final : public AbstractWindow {
    public:
        SDLWindow(SDL_Window *w, SDLDriver *dr, WindowFlags f);
        ~SDLWindow();

        // Overrides from WindowContext
        Size       size() const override;
        Point      position() const override;
        void       close() override;
        void       raise() override;
        void       resize(int w, int h) override;
        void       show(int v) override;
        void       move(int x, int y) override;
        void       set_title(u8string_view title) override;
        void       set_icon(const PixBuffer &buffer) override;
        void       repaint() override;

        void       capture_mouse(bool v) override;
        void       set_textinput_rect(const Rect &r) override;
        void       start_textinput(bool v) override;

        // Flags control
        bool       set_flags(WindowFlags flags) override;
        bool       set_value(int what, ...)   override;
        bool       query_value(int what, ...) override;

        Point      map_point(Point what, int type) override;

        widget_t   bind_widget(widget_t w) override;
        any_t      gc_create(const char_t *type) override;

        uint32_t   id() const;

        int        btk_to_sdl(int value) const;
        int        sdl_to_btk(int value) const;

        Point      btk_to_sdl(Point what) const;
        Point      sdl_to_btk(Point what) const;

        // Internal
        void       load_dpi(Uint32 display);
        void       do_repaint();
    private:
        float       vdpi = 96.0f;
        float       hdpi = 96.0f;
        float       ddpi = 96.0f;
        bool        dpi_scaling = true;

        SDL_Window *win = nullptr;
        SDLDriver  *driver = nullptr;
        Widget     *widget = nullptr;
        WindowFlags flags  = {};
        SDL_SysWMinfo info;

        // For merge paint events
        Uint32      last_repaint_ticks = 0; //< Last time request repaint ticks
        Uint32      last_paint_time = 0;    //< Last time the paint was doned
    friend class SDLDispatcher;
    friend class SDLDriver;
};

class SDLDispatcher final : public EventDispatcher {
    public:
        SDLDispatcher(SDLDriver *driver);
        ~SDLDispatcher();

        timerid_t timer_add(Object *obj, timertype_t, uint32_t ms) override;
        bool      timer_del(Object *obj, timerid_t id) override;

        pointer_t alloc(size_t n) override;
        bool      send(Event *event, EventDtor dtor) override;

        void      interrupt() override;
        int       run() override;
    private:
        bool      dispatch_sdl(SDL_Event *event);
        void      dispatch_sdl_window(SDL_Event *event);

        using TimersMap = std::unordered_map<timerid_t, SDLTimer>;

        SDLDriver *driver = nullptr;
        TimersMap  timers;

        Uint32    alloc_events = SDL_RegisterEvents(2);
        Uint32    btk_event    = alloc_events;
        Uint32    interrupt_event = alloc_events + 1;
};

class SDLDriver final : public GraphicsDriver {
    public:
        SDLDriver();
        ~SDLDriver();

        // Overrides from GraphicsDriver
        window_t window_create(u8string_view title, int width, int height,WindowFlags flags) override;
        void     window_add(SDLWindow *win);
        void     window_del(SDLWindow *win);

        void     clipboard_set(u8string_view str) override;
        u8string clipboard_get() override;

        cursor_t cursor_create(const PixBuffer &buf, int hot_x, int hot_y) override;
        cursor_t cursor_create(SystemCursor syscur) override;

        any_t    instance_create(const char_t *what, ...) override;

        pointer_t service_of(int what) override;

        bool      query_value(int what, ...) override;

#if     defined(_WIN32)
        Win32Dwmapi dwmapi;
#endif

    private:
        SDLDispatcher dispatcher {this};
        timestamp_t init_time = 0;

        float xdpi = 0;
        float ydpi = 0;
        
        // Map ID -> AbstractWindow
        std::unordered_map<uint32_t, SDLWindow *> windows;
    friend class SDLGraphicsContext;
    friend class SDLDispatcher;
};

class SDLGLContext final : public GLContext {
    public:
        SDLGLContext(SDL_Window *win);
        ~SDLGLContext();

        void begin() override;
        void end() override;
        void swap_buffers() override;

        bool      make_current() override;
        bool      initialize(const GLFormat &, GLContext *ctxt) override;
        bool      set_swap_interval(int v) override;
        Size      get_drawable_size() override;
        pointer_t get_proc_address(const char_t *name) override;
    private:    
        SDL_Window    *win = nullptr;
        SDL_GLContext  ctxt = nullptr;

        SDL_Window    *prev_win = nullptr;
        SDL_GLContext  prev_ctxt = nullptr;
};

// class SDLFbContext : public FbContext {
//     public:
//         SDLFbContext(SDL_Window *win);
//         ~SDLFbContext();

//         void begin() override;
//         void end() override;
//         void swap_buffers() override;

//         pointer_t get_pixels_address() override;
//         PixFormat get_pixels_format() override;
//         int       get_pixels_pitch() override;
//         Size      get_size() override;
//     private:
//         SDL_Window    *win = nullptr;
//         SDL_Surface   *surface = nullptr; //< Windows surface
// };

SDLDispatcher::SDLDispatcher(SDLDriver *d) : driver(d) {
    SetDispatcher(this);
}
SDLDispatcher::~SDLDispatcher() {
    if (GetDispatcher() == this) {
        SetDispatcher(nullptr);
    }
}
bool SDLDispatcher::dispatch_sdl(SDL_Event *event) {
    timestamp_t time = GetTicks();

    switch(event->type){
        case SDL_QUIT : {
            Event tr_event;
            tr_event.set_timestamp(GetTicks());
            tr_event.set_type(Event::Quit);
            interrupt();
            break;
        }
        case SDL_WINDOWEVENT : {
            dispatch_sdl_window(event);
            break;
        }
        case SDL_MOUSEMOTION : {
            SDLWindow *win = driver->windows[event->window.windowID];

            MotionEvent tr_event(
                win->sdl_to_btk(event->motion.x), 
                win->sdl_to_btk(event->motion.y)
            );
            tr_event.set_rel(
                win->sdl_to_btk(event->motion.xrel), 
                win->sdl_to_btk(event->motion.yrel)
            );
            tr_event.set_widget(win->widget);

            win->widget->handle(tr_event);
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            [[fallthrough]];
        }
        case SDL_MOUSEBUTTONUP: {
            SDLWindow *win = driver->windows[event->window.windowID];
            if (win == nullptr) {
                BTK_LOG("[SDL2] winid %d not found\n", event->window.windowID);
                break;
            }

            auto type = (event->type == SDL_MOUSEBUTTONDOWN) ? Event::MousePress : Event::MouseRelease;

            MouseEvent tr_event(
                type, 
                win->sdl_to_btk(event->button.x), 
                win->sdl_to_btk(event->button.y), 
                event->button.clicks
            );
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            switch (event->button.button) {
                case SDL_BUTTON_LEFT :
                    tr_event.set_button(MouseButton::Left);
                    break;
                case SDL_BUTTON_MIDDLE :
                    tr_event.set_button(MouseButton::Middle);
                    break;
                case SDL_BUTTON_RIGHT :
                    tr_event.set_button(MouseButton::Right);
                    break;
                case SDL_BUTTON_X1 :
                    tr_event.set_button(MouseButton::X1);
                    break;
                case SDL_BUTTON_X2 :
                    tr_event.set_button(MouseButton::X2);
                    break;
            }

            win->widget->handle(tr_event);
            break;
        }
        case SDL_KEYDOWN: {
            [[fallthrough]];
        }
        case SDL_KEYUP: {
            SDLWindow *win = driver->windows[event->window.windowID];
            auto type = (event->type == SDL_KEYDOWN) ? Event::KeyPress : Event::KeyRelease;

            auto keycode = SDLTranslateKey(event->key.keysym.sym);
            auto mod     = SDLTranslateMod(event->key.keysym.mod);

            KeyEvent tr_event(type, keycode, mod);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);
            tr_event.set_repeat(event->key.repeat);

            win->widget->handle(tr_event);
            break;
        }
        case SDL_MOUSEWHEEL : {
            SDLWindow *win = driver->windows[event->text.windowID];
            auto x = event->wheel.x;
            auto y = event->wheel.y;
            if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                x *= -1;
                y *= -1;
            }

            BTK_LOG("%d, %d\n", x, y);

            WheelEvent tr_event(x, y);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            win->widget->handle(tr_event);
            break;
        }
        case SDL_TEXTINPUT : {
            SDLWindow *win = driver->windows[event->text.windowID];
            
            TextInputEvent tr_event(event->text.text);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            win->widget->handle(tr_event);
            break;
        }
        default: {
            //BTK_LOG("unhandled event %d\n", event->type);
            return false;
        }
    }
    return true;
}
void SDLDispatcher::dispatch_sdl_window(SDL_Event *event) {
    // Get window from map
    auto iter = driver->windows.find(event->window.windowID);
    if (iter == driver->windows.end()) {
        return;
    }
    SDLWindow *win = iter->second;

    switch(event->window.event) {
        // case SDL_WINDOWEVENT_MOVED : {
        //     MoveEvent tr_event(event->window.data1, event->window.data2);
        //     tr_event.set_widget(win->widget);
        //     tr_event.set_timestamp(GetTicks());
        //     win->widget->handle(tr_event);
        //     break;
        // }
        case SDL_WINDOWEVENT_SIZE_CHANGED :
        case SDL_WINDOWEVENT_RESIZED : {
            int width = win->sdl_to_btk(event->window.data1);
            int height = win->sdl_to_btk(event->window.data2);

            ResizeEvent tr_event(width, height);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            win->widget->handle(tr_event);

            BTK_LOG(
                "[SDL2] Window resize to Physical (%d, %d) Logical (%d, %d)\n", 
                event->window.data1,
                event->window.data2,
                width, 
                height
            );
            break;
        }
        case SDL_WINDOWEVENT_EXPOSED : {
            PaintEvent tr_event;
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            win->widget->handle(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_ENTER : {
            [[fallthrough]];
        }
        case SDL_WINDOWEVENT_LEAVE : {
            auto type = (event->window.event == SDL_WINDOWEVENT_ENTER) ? Event::MouseEnter : Event::MouseLeave;
            int gx, gy;
            int x, y;

            // Get current mouse position
            SDL_GetMouseState(&gx, &gy);
            SDL_GetWindowPosition(win->win, &x, &y);

            MotionEvent tr_event(gx - x, gy - y);
            tr_event.set_widget(win->widget);
            tr_event.set_type(type);
            win->widget->handle(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_FOCUS_GAINED : {
            [[fallthrough]];
        }
        case SDL_WINDOWEVENT_FOCUS_LOST : {
            auto type = (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) ? Event::FocusGained : Event::FocusLost;
            WidgetEvent tr_event(type);
            tr_event.set_widget(win->widget);
            win->widget->handle(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_CLOSE : {
            CloseEvent event;
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            event.accept();
            win->widget->handle(event);

            if (event.is_accepted()) {
                SDL_HideWindow(win->win);
                driver->windows.erase(win->id());

                if (driver->windows.empty()) {
                    interrupt();
                }
            }
            break;
        }
#if     SDL_VERSION_ATLEAST(2, 0, 15)
        case SDL_WINDOWEVENT_DISPLAY_CHANGED : {
            // We should reload dpi on display changed
            // May bug here?
            auto [w, h] = win->size();
            win->load_dpi(event->window.data1);
            win->resize(w, h);
            break;
        }
#endif
        default : {
            break;
        }
    }
}

int SDLDispatcher::run() {
    int retcode = EXIT_SUCCESS;

    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (!dispatch_sdl(&event)) {
            // It cannot handle it, event defined by ous

            if (event.type == btk_event) {
                auto btkevent = reinterpret_cast<Event*>(event.user.data1);
                auto dtor = reinterpret_cast<EventDtor>(event.user.data2);

                dispatch(btkevent);

                if (dtor) {
                    dtor(btkevent);
                }

                SDL_free(btkevent);
                continue;
            }
            if (event.type == interrupt_event) {
                break;
            }
            if (event.type == SDL_TIMER_EVENT) {
                auto timerid = reinterpret_cast<timerid_t>(event.user.data1);
                auto timestamp = reinterpret_cast<timestamp_t>(event.user.data2);
                auto iter = timers.find(timerid);
                if (iter != timers.end()) {
                    auto object = iter->second.object;
                    TimerEvent tevent(
                        object,
                        timerid
                    );
                    tevent.set_timestamp(timestamp);

                    dispatch(&tevent);
                }
                continue;
            }
            if (event.type == SDL_REPAINT_EVENT) {
                auto winid = event.user.windowID;
                auto timestamp = reinterpret_cast<timestamp_t>(event.user.data1);
                auto iter = driver->windows.find(winid);
                if (iter!= driver->windows.end()) {
                    iter->second->do_repaint();
                }
                continue;
            }
        }
    }

    return retcode;
}
void SDLDispatcher::interrupt() {
    SDL_Event event;
    event.type = interrupt_event;
    SDL_PushEvent(&event);
}
void*SDLDispatcher::alloc(size_t n) {
    return SDL_malloc(n);
}
bool SDLDispatcher::send(Event *event, EventDtor dtor) {
    SDL_Event sdlevent;
    sdlevent.type = btk_event;
    sdlevent.user.data1 = event;
    sdlevent.user.data2 = reinterpret_cast<void*>(dtor);

    return SDL_PushEvent(&sdlevent) == 1;
}

bool SDLDispatcher::timer_del(Object *obj, timerid_t id) {
    BTK_UNUSED(obj);

    auto iter = timers.find(id);
    if (iter != timers.end()) {
        auto timer = iter->second.timer;
        timers.erase(iter);
        return SDL_RemoveTimer(timer);
    }
    return false;
}
timerid_t SDLDispatcher::timer_add(Object *obj, timertype_t, uint32_t ms) {
    // Alloc id
    timerid_t id;
    do {
        id = std::rand();
    }
    while(timers.find(id) != timers.end());  

    SDL_TimerID timer = SDL_AddTimer(ms, [](Uint32 interval, void *id) -> Uint32 {
        SDL_Event event;
        event.type = SDL_TIMER_EVENT;
        event.user.data1 = id;
        event.user.data2 = reinterpret_cast<void*>(GetTicks());

        SDL_PushEvent(&event);
        return interval;
    }, reinterpret_cast<void*>(id));

    if (timer == 0) {
        return 0;
    }

    timers[id].object = obj;
    timers[id].timer  = timer;

    return id;
}

// Driver
SDLDriver::SDLDriver() {
    // TODO IMPL IT
#if defined(SDL_HINT_WINDOWS_DPI_AWARENESS)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

#if defined(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR)
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif

    SDL_Init(SDL_INIT_VIDEO);
    init_time = GetTicks();

    // Query DPI
    SDL_GetDisplayDPI(0, nullptr, &xdpi, &ydpi);

    BTK_LOG(" xdpi : %f, ydpi : %f\n", xdpi, ydpi);

    BtkBacktraceInit();
}
SDLDriver::~SDLDriver() {
    SDL_Quit();
}
window_t SDLDriver::window_create(u8string_view title, int width, int height, WindowFlags flags) {
    // Translate flags to SDL flags
    uint32_t sdl_flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if ((flags & WindowFlags::Resizable) == WindowFlags::Resizable) {
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    }
    if ((flags & WindowFlags::OpenGL) == WindowFlags::OpenGL) {
        sdl_flags |= SDL_WINDOW_OPENGL;
    }

#if defined(__gnu_linux__) || defined(BTK_NANOVG_PAINTER)
    sdl_flags |= SDL_WINDOW_OPENGL;
#endif

    SDL_Window *win = SDL_CreateWindow(
        u8string(title).c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        sdl_flags | SDL_WINDOW_HIDDEN
    );
    auto ret = new SDLWindow(win, this, flags);

    // Resize by dpi
    ret->resize(width, height);

    // Framebuffer means that the window is always invisable, no need to add it to event dispatcher
    if ((flags & WindowFlags::Framebuffer) != WindowFlags::Framebuffer) {
        window_add(ret);
    }
    return ret;
}
void    SDLDriver::window_add(SDLWindow *win) {
    windows[win->id()] = win;
}
void    SDLDriver::window_del(SDLWindow *win) {
    windows.erase(win->id());

    // Maybe we should walk through the event queue and remove all events for this window?
}
u8string SDLDriver::clipboard_get() {
    char *str = SDL_GetClipboardText();
    u8string ret = u8string(str);
    SDL_free(str);
    return ret;
}
void SDLDriver::clipboard_set(u8string_view str) {
    SDL_SetClipboardText(str.copy().c_str());
}
bool   SDLDriver::query_value(int what, ...) {
    va_list varg;
    va_start(varg, what);
    bool ret = true;

    switch (what) {
        // case SystemDpi : {

        // }
        default : {
            ret = false;
            break;
        }
    }

    va_end(varg);
    return ret;
}
any_t SDLDriver::instance_create(const char_t *what, ...) {
    va_list varg;
    va_start(varg, what);
    any_t ret = nullptr;

    if (SDL_strcasecmp(what, "filedialog") == 0) {

#if    defined(_WIN32)
        ret = new Win32FileDialog;
#elif  defined(__gnu_linux__)
        if (X11FileDialog::Supported()) {
            ret = new X11FileDialog();
        }
#endif


    }


    va_end(varg);
    return ret;
}
cursor_t SDLDriver::cursor_create(const PixBuffer &buf, int hot_x, int hot_y) {
    return nullptr;
}
cursor_t SDLDriver::cursor_create(SystemCursor syscur) {
    return nullptr;
}
pointer_t SDLDriver::service_of(int what) {
    return nullptr;
}

// Window
SDLWindow::SDLWindow(SDL_Window *w, SDLDriver *dr, WindowFlags f) : win(w), driver(dr), flags(f) {
    SDL_GetVersion(&info.version);

    if (!SDL_GetWindowWMInfo(win, &info)) {
        BTK_THROW(std::runtime_error(SDL_GetError()));
    }

    auto idx = SDL_GetWindowDisplayIndex(win);
    load_dpi(idx);

#if defined(_WIN32)
    driver->dwmapi.DwmEnableAlphaBlend(info.info.win.window);
#endif
}
SDLWindow::~SDLWindow() {
    // Remove self from driver
    driver->window_del(this);
    SDL_DestroyWindow(win);
}
Size SDLWindow::size() const {
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    w = sdl_to_btk(w);
    h = sdl_to_btk(h);
    return Size(w, h);
}
Point SDLWindow::position() const {
    int x, y;
    SDL_GetWindowPosition(win, &x, &y);
    return Point(x, y);
}

void SDLWindow::move(int x, int y) {
    SDL_SetWindowPosition(win, x, y);
}
void SDLWindow::resize(int w, int h) {
    w = btk_to_sdl(w);
    h = btk_to_sdl(h);
    SDL_SetWindowSize(win, w, h);
}
void SDLWindow::show(int v) {
    switch (v) {
        default : [[fallthrough]];
        case Show : SDL_ShowWindow(win); break;
        case Hide : SDL_HideWindow(win); break;
        case Restore  : SDL_RestoreWindow(win); break;
        case Maximize : SDL_MaximizeWindow(win); break;
        case Minimize : SDL_MinimizeWindow(win); break;
    }
}
void SDLWindow::raise() {
    SDL_RaiseWindow(win);
}
void SDLWindow::close() {
    SDL_Event event;
    SDL_zero(event);

    event.window.event = SDL_WINDOWEVENT_CLOSE;
    event.window.timestamp = SDL_GetTicks();
    event.window.type = SDL_WINDOWEVENT;
    event.window.windowID = SDL_GetWindowID(win);

    SDL_PushEvent(&event);
}
void SDLWindow::repaint() {

#if defined(_WIN32)
    InvalidateRect(info.info.win.window, nullptr, FALSE);
#else
    SDL_Event event;
    SDL_zero(event);

    // event.window.event = SDL_WINDOWEVENT_EXPOSED;
    // event.window.timestamp = SDL_GetTicks();
    // event.window.type = SDL_WINDOWEVENT;
    // event.window.windowID = SDL_GetWindowID(win);
    Uint32 current = SDL_GetTicks();
    if (current - last_repaint_ticks < 1) {
        // Merge
        return;
    }
    last_repaint_ticks = current;

    event.type = SDL_REPAINT_EVENT;
    event.user.windowID = SDL_GetWindowID(win);

    SDL_PushEvent(&event);
#endif
}
void SDLWindow::set_title(u8string_view title) {
    SDL_SetWindowTitle(win, title.copy().c_str());
}
void SDLWindow::set_icon(const PixBuffer &buf) {
    SDL_Surface *ref = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<void *>(buf.pixels()),
        buf.width(),
        buf.height(),
        buf.bpp(),
        buf.pitch(),
        buf.bpp() == 32 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24
    );
    SDL_SetWindowIcon(win, ref);
    SDL_FreeSurface(ref);

}
void SDLWindow::capture_mouse(bool v) {
    SDL_CaptureMouse(SDL_bool(v));
}
void SDLWindow::start_textinput(bool v) {
    if (v) {
        SDL_StartTextInput();
    }
    else {
        SDL_StopTextInput();
    }
}
void SDLWindow::set_textinput_rect(const Rect &r) {
    SDL_SetTextInputRect((SDL_Rect*) &r);
}
bool SDLWindow::set_flags(WindowFlags new_flags) {
    auto bit_changed = [=](WindowFlags what) {
        return (flags & what) != (new_flags & what);
    };
    int err = 0;

    if (bit_changed(WindowFlags::Resizable)) {
        SDL_SetWindowResizable(win, SDL_bool(new_flags & WindowFlags::Resizable));
    }
    if (bit_changed(WindowFlags::Fullscreen)) {
        if (bool(new_flags & WindowFlags::Fullscreen)) {
            err |= SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        else {
            err |= SDL_SetWindowFullscreen(win, 0);
        }
    }
    if (bit_changed(WindowFlags::Borderless)) {
        if (bool(new_flags & WindowFlags::Borderless)) {
            SDL_SetWindowBordered(win, SDL_FALSE);
        }
        else {
            SDL_SetWindowBordered(win, SDL_TRUE);
        }
    }

    flags = new_flags;
    return err == 0;
}
widget_t SDLWindow::bind_widget(widget_t widget) {
    widget_t ret = this->widget;
    this->widget = widget;
    return ret;
}
bool     SDLWindow::query_value(int what, ...) {
    va_list varg;
    va_start(varg, what);
    bool ret = true;

    switch (what) {
        // Platform data query
#if defined(_WIN32)
        case NativeHandle : 
        case Hwnd :
            *va_arg(varg, HWND*) = info.info.win.window;
            break;
        case Hdc :
            *va_arg(varg, HDC*) = info.info.win.hdc; 
            break;
#elif defined(__gnu_linux__)
        case NativeHandle :
        case XWindow :
            *va_arg(varg, Window*) = info.info.x11.window;
            break;
        case XDisplay :
            *va_arg(varg, Display**) = info.info.x11.display;
            break;
#else
#error "Unsupport backend"
#endif
        case MousePosition : {
            int x, y;
            // int wx, wy;
            SDL_GetMouseState(&x, &y);
            // SDL_GetWindowPosition(win, &wx, &wy);

            auto p = va_arg(varg, Point*);
            p->x = sdl_to_btk(x);
            p->y = sdl_to_btk(y);
            break;
        }
        case Dpi : {            
            auto p = va_arg(varg, FPoint*);
            p->x = hdpi;
            p->y = vdpi;
            break;
        }
        case MaximumSize : {
            auto p = va_arg(varg, Size*);
            SDL_GetWindowMaximumSize(win, &p->w, &p->h);

            p->w = sdl_to_btk(p->w);
            p->h = sdl_to_btk(p->h);
            break;
        }
        case MinimumSize : {
            auto p = va_arg(varg, Size*);
            SDL_GetWindowMinimumSize(win, &p->w, &p->h);

            p->w = sdl_to_btk(p->w);
            p->h = sdl_to_btk(p->h);
            break;
        }
        case Opacity : {
            ret = (SDL_GetWindowOpacity(win, va_arg(varg, float*)) == 0);
            break;
        }
        case Value::SDLWindow : {
            auto p = va_arg(varg, SDL_Window**);
            *p = win;
            break;
        }
        default : {
            ret = false;
            break;
        }
    }

// #if defined(_WIN32)
//     if (what == NativeHandle || what == Hwnd) {
//         return info.info.win.window;
//     }
//     if (what == Hdc) {
//         return info.info.win.hdc;
//     }
// #elif defined(__gnu_linux__)
//     if (what == NativeHandle || what == XWindow) {
//         return reinterpret_cast<pointer_t>(info.info.x11.window);
//     }
//     if (what == XDisplay) {
//         return info.info.x11.display;
//     }
// #else
// #error "Unsupport backend"
// #endif
    va_end(varg);
    return ret;
}
bool    SDLWindow::set_value(int what, ...) {
    va_list varg;
    va_start(varg, what);
    bool ret = true;

    switch (what) {
        case MaximumSize : {
            auto [w, h] = *va_arg(varg, Size*);
            w = btk_to_sdl(w);
            h = btk_to_sdl(h);

            SDL_SetWindowMaximumSize(win, w, h);
            break;
        }
        case MinimumSize : {
            auto [w, h] = *va_arg(varg, Size*);
            w = btk_to_sdl(w);
            h = btk_to_sdl(h);

            SDL_SetWindowMinimumSize(win, w, h);
            break;
        }
        case MousePosition : {
            auto [x, y] = *va_arg(varg, Point*);
            SDL_WarpMouseInWindow(win, btk_to_sdl(x), btk_to_sdl(y));
            break;
        }
        case Opacity : {
            ret = (SDL_SetWindowOpacity(win, *va_arg(varg, float*)) == 0);
            break;
        }
        default : {
            ret = false;
            break;
        }
    }
    va_end(varg);
    return ret;
}
int     SDLWindow::sdl_to_btk(int v) const {
    if (!dpi_scaling) {
        return v;
    }
    return muldiv<int>(v, 96, hdpi);
}
int     SDLWindow::btk_to_sdl(int v) const {
    if (!dpi_scaling) {
        return v;
    }
    return muldiv<int>(v, hdpi, 96);
}
Point   SDLWindow::sdl_to_btk(Point p) const {
    if (dpi_scaling) {
        p.x = muldiv<int>(p.x, 96, hdpi);
        p.y = muldiv<int>(p.y, 96, vdpi);
    }
    return p;
}
Point   SDLWindow::btk_to_sdl(Point p) const {
    if (dpi_scaling) {
        p.x = muldiv<int>(p.x, hdpi, 96);
        p.y = muldiv<int>(p.y, vdpi, 96);
    }
    return p;
}
Point   SDLWindow::map_point(Point p, int type) {
    switch (type) {
        case ToScreen : {
            // Convert to pixels
            int wx, wy;
            SDL_GetWindowPosition(win, &wx, &wy);
            p = btk_to_sdl(p);

            return Point(p.x + wx, p.y + wy);
        }
        case ToClient : {
            int wx, wy;
            SDL_GetWindowPosition(win, &wx, &wy);
            p = btk_to_sdl(p);

            return Point(p.x - wx, p.y - wy);
        }
        case ToPixel : {
            return btk_to_sdl(p);
        }
        case ToDIPS : {
            return sdl_to_btk(p);
        }
        default : {
            // Unsupported
            return p;
        }
    }
}
any_t   SDLWindow::gc_create(const char_t *what) {
    // Create GC
    if (SDL_strcasecmp(what, "opengl") == 0) {
        return new SDLGLContext(win);
    }
    if (SDL_strcasecmp(what, "vulkan") == 0) {
        return nullptr;
    }
    if (SDL_strcasecmp(what, "metal") == 0) {
        return nullptr;
    }
    if (SDL_strcasecmp(what, "framebuffer") == 0) {
        // Get Framebuffer
        // return new SDLFbContext(win);
        return nullptr;
    }    
    return nullptr;
}
void   SDLWindow::load_dpi(Uint32 idx) {
    if (SDL_GetDisplayDPI(idx, &ddpi, &hdpi, &vdpi) != 0) {
        BTK_LOG("[SDL2] Display %d no dpi information, backward to 96\n", idx);
        ddpi = 96.0f;
        hdpi = 96.0f;
        vdpi = 96.0f;
    }
    BTK_LOG("[SDL2] Display %d ddpi %f hdpi %f vdpi %f\n", idx, ddpi, hdpi, vdpi);

#if 1
    if (hdpi < 96.0) {
        dpi_scaling = false;
    }
#endif
}
void   SDLWindow::do_repaint() {
    PaintEvent event;
    event.set_widget(widget);
    event.set_timestamp(GetTicks());

    widget->handle(event);
    
    last_paint_time = SDL_GetTicks();
}

uint32_t SDLWindow::id() const {
    return SDL_GetWindowID(win);
}

SDLGLContext::SDLGLContext(SDL_Window *win) : win(win) {}
SDLGLContext::~SDLGLContext() {
    if (ctxt) {
        SDL_GL_DeleteContext(ctxt);
    }
}

bool SDLGLContext::initialize(const GLFormat &format, GLContext *share) {
    // Share if
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, share != nullptr);
    if (share) {
        if (!share->make_current()) {
            return false;
        }
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, format.red_size);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, format.green_size);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, format.blue_size);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, format.alpha_size);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, format.depth_size);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, format.stencil_size);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, format.sample_buffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, format.samples);

    ctxt = SDL_GL_CreateContext(win);
    if (!ctxt) {
        return false;
    }
    return true;
}
void SDLGLContext::begin() {
    prev_ctxt = SDL_GL_GetCurrentContext();
    if (prev_ctxt != ctxt) {
        prev_win = SDL_GL_GetCurrentWindow();
        SDL_GL_MakeCurrent(win, ctxt);
    }
    else {
        prev_win = nullptr;
    }
}
void SDLGLContext::end() {
    if (prev_win) {
        SDL_GL_MakeCurrent(prev_win, prev_ctxt);
    }
}
void SDLGLContext::swap_buffers() {
    SDL_GL_SwapWindow(win);
}
bool SDLGLContext::make_current() {
    return SDL_GL_MakeCurrent(win, ctxt);
}
bool SDLGLContext::set_swap_interval(int interval) {
    return SDL_GL_SetSwapInterval(interval) == 0;
}
Size SDLGLContext::get_drawable_size() {
    begin();
    Size s;
    SDL_GL_GetDrawableSize(win, &s.w, &s.h);
    end();
    return s;
}
pointer_t SDLGLContext::get_proc_address(const char_t *name) {
    begin();
    pointer_t ret = SDL_GL_GetProcAddress(name);
    end();
    return ret;
}

// Framebuffer Context
// SDLFbContext::SDLFbContext(SDL_Window *w) : win(w) {}
// SDLFbContext::~SDLFbContext() {
//     SDL_FreeSurface(surface);
// }

// void SDLFbContext::begin() {
//     if (surface) {
//         int w, h;
//         SDL_GetWindowSize(win, &w, &h);
//         if (surface->w != w || surface->h != h) {
//             SDL_FreeSurface(surface);
//             surface = nullptr;
//         }
//     }
//     if (!surface) {
//         surface = SDL_GetWindowSurface(win);
//     }
//     if (!surface) {
//         return;
//     }

//     if (SDL_MUSTLOCK(surface)) {
//         SDL_LockSurface(surface);
//     } 
// }
// void SDLFbContext::end() {
//     // Nothing to do here
//     if (SDL_MUSTLOCK(surface)) { 
//         SDL_UnlockSurface(surface);
//     }
// }
// void SDLFbContext::swap_buffers() {
//     SDL_UpdateWindowSurface(win);
// }


// pointer_t SDLFbContext::get_pixels_address() {
//     if (!surface) {
//         return nullptr;
//     }
//     return surface->pixels;
// }
// PixFormat SDLFbContext::get_pixels_format() {
//     if (!surface) {
//         return {};
//     }
//     switch (surface->format->format) {
//         case SDL_PIXELFORMAT_BGR24 : return PixFormat::BGR24;
//         case SDL_PIXELFORMAT_RGB24 : return PixFormat::RGB24;
//         case SDL_PIXELFORMAT_RGBA32 : return PixFormat::RGBA32;
//     }
//     return {};
// }
// int       SDLFbContext::get_pixels_pitch() {
//     if (!surface) {
//         return 0;
//     }
//     return surface->pitch;
// }
// Size       SDLFbContext::get_size() {
//     if (!surface) {
//         return {};
//     }
//     return Size(surface->w, surface->h);
// }

BTK_PRIV_END

BTK_NS_BEGIN

GraphicsDriverInfo SDLDriverInfo = {
    []() -> GraphicsDriver * {
        return new SDLDriver();
    },
    "SDL2",
};

BTK_NS_END