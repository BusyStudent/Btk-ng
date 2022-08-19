#include "build.hpp"

#include <Btk/context.hpp>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL.h>
#include <unordered_map>

// Remove xlib macro 

#undef KeyPress
#undef KeyRelease
#undef None

// Internal use helper functions

#include "sdl2_trkey.hpp"

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

// Ability of SDL_Renderer

#define SDL_GC_FLOAT  SDL_VERSION_ATLEAST(2, 0, 1)
#define SDL_GC_VERTEX SDL_VERSION_ATLEAST(2, 0, 18)

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
class SDLFont   ;
class SDLWindow : public AbstractWindow {
    public:
        SDLWindow(SDL_Window *w, SDLDriver *dr) : win(w), driver(dr) {}
        ~SDLWindow();

        // Overrides from WindowContext
        Size       size() const override;
        Point      position() const override;
        void       close() override;
        void       raise() override;
        void       resize(int w, int h) override;
        void       show(bool v) override;
        void       move(int x, int y) override;
        void       set_title(const char_t *title) override;
        void       set_icon(const PixBuffer &buffer) override;
        void       repaint() override;

        void       capture_mouse(bool v) override;
        void       set_textinput_rect(const Rect &r) override;
        void       start_textinput(bool v) override;

        pointer_t  native_handle(int what) override;
        widget_t   bind_widget(widget_t w) override;
        any_t      gc_create(const char_t *type) override;

        uint32_t   id() const;
    private:
        SDL_Window *win = nullptr;
        SDLDriver  *driver = nullptr;
        Widget     *widget = nullptr;
    friend class SDLDriver;
};

class SDLDriver : public GraphicsDriver {
    public:
        SDLDriver();
        ~SDLDriver();

        // Overrides from GraphicsDriver
        window_t window_create(const char_t * title, int width, int height,WindowFlags flags) override;
        void     window_add(SDLWindow *win);
        void     window_del(SDLWindow *win);

        void     clipboard_set(const char_t *str) override;
        u8string clipboard_get() override;

        void     pump_events(UIContext *) override;

        int      sdl_event_filter(SDL_Event *event);
        void     tr_window_event(SDL_Event *evemt);

        timerid_t timer_add(Object *obj, timertype_t, uint32_t ms) override;
        bool      timer_del(Object *obj, timerid_t id) override;

    private:
        timestamp_t init_time = 0;
        UIContext  *context = nullptr;
        EventQueue *event_queue = nullptr;

        float xdpi = 0;
        float ydpi = 0;
        
        // Map ID -> AbstractWindow
        std::unordered_map<uint32_t, SDLWindow *> windows;
    friend class SDLGraphicsContext;
};

class SDLGLContext : public GLContext {
    public:
        SDLGLContext(SDL_Window *win);
        ~SDLGLContext();

        void begin() override;
        void end() override;
        void swap_buffers() override;

        bool      initialize(const GLFormat &) override;
        bool      set_swap_interval(int v) override;
        Size      get_drawable_size() override;
        pointer_t get_proc_address(const char_t *name) override;
    private:    
        SDL_Window    *win = nullptr;
        SDL_GLContext  ctxt = nullptr;

        SDL_Window    *prev_win = nullptr;
        SDL_GLContext  prev_ctxt = nullptr;
};


// Driver

SDLDriver::SDLDriver() {
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    SDL_Init(SDL_INIT_VIDEO);
    init_time = GetTicks();

    // Query DPI
    SDL_GetDisplayDPI(0, nullptr, &xdpi, &ydpi);

    BTK_LOG(" xdpi : %f, ydpi : %f\n", xdpi, ydpi);
}
SDLDriver::~SDLDriver() {
    SDL_Quit();
}

int SDLDriver::sdl_event_filter(SDL_Event *event) {
    timestamp_t time = GetTicks();
    

    switch(event->type){
        case SDL_QUIT : {
            Event tr_event;
            tr_event.set_timestamp(GetTicks());
            tr_event.set_type(Event::Quit);
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT : {
            tr_window_event(event);
            break;
        }
        case SDL_MOUSEMOTION : {
            SDLWindow *win = windows[event->window.windowID];

            MotionEvent tr_event(event->motion.x, event->motion.y);
            tr_event.set_rel(event->motion.xrel, event->motion.yrel);
            tr_event.set_widget(win->widget);

            win->widget->handle(tr_event);
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            [[fallthrough]];
        }
        case SDL_MOUSEBUTTONUP: {
            SDLWindow *win = windows[event->window.windowID];
            if (win == nullptr) {
                BTK_LOG("winid %d not found\n", event->window.windowID);
                break;
            }

            auto type = (event->type == SDL_MOUSEBUTTONDOWN) ? Event::MousePress : Event::MouseRelease;

            MouseEvent tr_event(type, event->button.x, event->button.y, event->button.clicks);
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
            SDLWindow *win = windows[event->window.windowID];
            auto type = (event->type == SDL_KEYDOWN) ? Event::KeyPress : Event::KeyRelease;

            auto keycode = SDLTranslateKey(event->key.keysym.sym);
            auto mod     = SDLTranslateMod(event->key.keysym.mod);

            KeyEvent tr_event(type, keycode, mod);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            win->widget->handle(tr_event);
            break;
        }
        case SDL_MOUSEWHEEL : {
            SDLWindow *win = windows[event->text.windowID];
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
            SDLWindow *win = windows[event->text.windowID];
            
            TextInputEvent tr_event(event->text.text);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            win->widget->handle(tr_event);
            break;
        }
        default: {
            //BTK_LOG("unhandled event %d\n", event->type);
        }
    }
    return 0;
}
void SDLDriver::tr_window_event(SDL_Event *event) {
    // Get window from map
    auto iter = windows.find(event->window.windowID);
    if (iter == windows.end()) {
        return;
    }
    SDLWindow *win = iter->second;

    switch(event->window.event) {
        case SDL_WINDOWEVENT_MOVED : {
            MoveEvent tr_event(event->window.data1, event->window.data2);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            win->widget->handle(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_RESIZED : {
            ResizeEvent tr_event(event->window.data1, event->window.data2);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            win->widget->handle(tr_event);

            printf("Resize : %d, %d\n", event->window.data1, event->window.data2);
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
                windows.erase(win->id());

                if (windows.empty()) {
                    QuitEvent event;
                    context->send_event(event);
                }
            }
        }
        default : {
            break;
        }
    }
}

void SDLDriver::pump_events(UIContext *ctxt) {
    context = ctxt;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        sdl_event_filter(&event);
    }
}

window_t SDLDriver::window_create(const char_t * title, int width, int height,WindowFlags flags){
    // Translate flags to SDL flags
    uint32_t sdl_flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if ((flags & WindowFlags::Resizable) == WindowFlags::Resizable) {
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    }

#if defined(__gnu_linux__)
    sdl_flags |= SDL_WINDOW_OPENGL;
#endif

    SDL_Window *win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        sdl_flags | SDL_WINDOW_HIDDEN
    );
    auto ret = new SDLWindow(win, this);
    window_add(ret);
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
void SDLDriver::clipboard_set(const char_t *str) {
    SDL_SetClipboardText(str);
}
bool SDLDriver::timer_del(Object *obj, timerid_t id) {
    BTK_UNUSED(obj);

    auto ctxt = context;
    if (ctxt == nullptr) {
        ctxt = GetUIContext();
    }
    bool v = SDL_RemoveTimer(id) == SDL_TRUE;

    // Walk queue, drop timer if it's in there
    ctxt->walk_event([&](Event &ev){
        if (ev.type() == Event::Timer) {
            auto tev = static_cast<TimerEvent&>(ev);
            if (tev.timerid() == id) {
                return EventWalk::Drop;
            }
        }
        return EventWalk::Continue;
    });
    return v;
}
timerid_t SDLDriver::timer_add(Object *obj, timertype_t, uint32_t ms) {
    struct TimerData {
        UIContext *ctxt;
        Object *obj;
        timerid_t id;
    };
    auto data = new TimerData();
    data->obj = obj;
    data->ctxt = context;
    data->id = 0;

    if (data->ctxt == nullptr) {
        data->ctxt = GetUIContext();
    }
    BTK_ASSERT_MSG(data->ctxt, "No UI context");

    //Add sdl_timer
    auto cb = [](uint32_t ms,void *user) {
        auto up = static_cast<TimerData*>(user);
        auto ctxt = up->ctxt;
        auto obj = up->obj;
        auto id  = up->id;

        if (id != 0) {
            // BTK_LOG("SDL: Send timer event\n");

            TimerEvent event(obj, id);
            event.set_timestamp(GetTicks());
            ctxt->send_event(event);
        }

        return ms;
    };
    auto id = SDL_AddTimer(ms, cb, data);
    if (id == 0) {
        BTK_LOG("SDL: Failed to start timer => %s\n", SDL_GetError());
        delete data;
    }
    data->id = id;
    return id;
}

// Window

SDLWindow::~SDLWindow() {
    // Remove self from driver
    driver->window_del(this);
    SDL_DestroyWindow(win);
}
Size SDLWindow::size() const {
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
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
    SDL_SetWindowSize(win, w, h);
}
void SDLWindow::show(bool v) {
    if (v) {
        SDL_ShowWindow(win);
    }
    else {
        SDL_HideWindow(win);
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
    SDL_Event event;
    SDL_zero(event);

    event.window.event = SDL_WINDOWEVENT_EXPOSED;
    event.window.timestamp = SDL_GetTicks();
    event.window.type = SDL_WINDOWEVENT;
    event.window.windowID = SDL_GetWindowID(win);

    SDL_PushEvent(&event);
}
void SDLWindow::set_title(const char_t *title) {
    SDL_SetWindowTitle(win, title);
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
widget_t SDLWindow::bind_widget(widget_t widget) {
    widget_t ret = this->widget;
    this->widget = widget;
    return ret;
}
pointer_t SDLWindow::native_handle(int what) {
    SDL_SysWMinfo info;
    SDL_GetVersion(&info.version);

    if (!SDL_GetWindowWMInfo(win, &info)) {
        BTK_THROW(std::runtime_error(SDL_GetError()));
    }

#if defined(_WIN32)
    if (what == NativeHandle || what == Hwnd) {
        return info.info.win.window;
    }
#elif defined(__gnu_linux__)
    if (what == NativeHandle || what == XWindow) {
        return reinterpret_cast<pointer_t>(info.info.x11.window);
    }
    if (what == XDisplay) {
        return info.info.x11.display;
    }
#else
#error "Unsupport backend"
#endif
    // Unmatched
    return nullptr;
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
        return nullptr;
    }    
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

bool SDLGLContext::initialize(const GLFormat &format) {
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

BTK_NS_END2()

BTK_NS_BEGIN

GraphicsDriverInfo SDLDriverInfo = {
    []() -> GraphicsDriver * {
        return new SDLDriver();
    },
    "SDL2",
};

BTK_NS_END