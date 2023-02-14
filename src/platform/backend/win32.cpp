#include "build.hpp"
#include "common/win32/backtrace.hpp"
#include "common/win32/dialog.hpp"
#include "common/dlloader.hpp"

#include <Btk/service/desktop.hpp>
#include <Btk/detail/platform.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/context.hpp>
#include <Btk/event.hpp>
#include <unordered_map>
#include <Windows.h>
#include <wingdi.h>
#include <ShlObj.h>
#include <csignal>
#include <memory>
#include <stack>
#include <tuple>
#include <wrl.h>

#if defined(_MSC_VER)
    #pragma comment(linker,"\"/manifestdependency:type='win32' \
        name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
        processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
    #pragma comment(lib, "shell32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "gdi32.lib")
    #pragma comment(lib, "imm32.lib")
#endif

#undef min
#undef max

// Event
#define BTK_CALL    (WM_APP + 0x100)
#define BTK_EVENT   (WM_APP + 0x101)

// Class
#define BTK_WINDOW_CLASS L"BtkWindow"
#define BTK_HELPER_CLASS L"BtkHelper"

// OpenGL support
#define GL_LIB(x)    HMODULE library = LoadLibraryA(x)
#define GL_TYPE(x)   decltype(::x)
#define GL_PROC(x)   GL_TYPE(x) *x = (GL_TYPE(x)*) GetProcAddress(library, #x) 
#define GL_WPROC(x)  PFN##x x = nullptr

// Timer support
#define TIMER_PRECISE (UINTPTR_MAX / 2)
#define TIMER_COARSE  (              0)

#define TIMER_IS_PRECISE(x) ((x) >= TIMER_PRECISE)
#define TIMER_IS_COARSE(x)  ((x) <  TIMER_PRECISE)

// Debug show key translation strings
#if 0
#define SHOW_KEY(vk, k) printf(vk); printf(" => "); printf(k); printf("\n");
#define MAP_KEY(vk, k) case vk : { SHOW_KEY(#vk, #k) ; keycode = k; break;}
#else
#define MAP_KEY(vk, k) case vk : { keycode = k; break; }
#endif

#define WIN_LOG(...) BTK_LOG(__VA_ARGS__)


// HDPI API Support
#if !defined(_DPI_AWARENESS_CONTEXTS_)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE     ((DPI_AWARENESS_CONTEXT)-3)
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif


using Microsoft::WRL::ComPtr;
using Win32Callback = void (*)(void *param);

// WGL pfn
using PFNwglChoosePixelFormatARB = BOOL (WINAPI *)(HDC h, const int *attr, const FLOAT *fattr, UINT n, int *pf, UINT *nf);
using PFNwglCreateContextAttribsARB = HGLRC (WINAPI *)(HDC h, HGLRC share, const int *attr);
using PFNwglGetExtensionsStringARB = const char *(WINAPI *)(HDC h);
using PFNwglSetSwapIntervalEXT = BOOL (WINAPI *)(int interval);

// User32 library
LIB_BEGIN(Win32User32, "user32.dll")
    // 
    LIB_PROC4(BOOL(* WINAPI)(LPRECT, DWORD, BOOL, DWORD, UINT), AdjustWindowRectExForDpi)
    LIB_PROC4(BOOL(* WINAPI)(DPI_AWARENESS_CONTEXT), SetProcessDpiAwarenessContext)
    LIB_PROC4(UINT(* WINAPI)(HWND), GetDpiForWindow)
    LIB_PROC4(UINT(* WINAPI)()    , GetDpiForSystem)
LIB_END(Win32User32)

// Make sure the MSG can be casted to a Win32Callback
static_assert(sizeof(LPARAM) >= sizeof(void *));
static_assert(sizeof(WPARAM) >= sizeof(void *));
// Make sure is utf16
static_assert(sizeof(wchar_t) == sizeof(uint16_t));

BTK_PRIV_BEGIN

class Win32Driver ;
class Win32Window ;

// For timer stored at object
class Win32Timer  {
    public:
        HANDLE  handle = INVALID_HANDLE_VALUE; //< Valid if CreateTimerQueueTimer 
        Object *object = nullptr;
};

class Win32Window final : public AbstractWindow {
    public:
        Win32Window(HWND h, Win32Driver *d);
        ~Win32Window();

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

        // IME support
        void       start_textinput(bool v) override;
        void       set_textinput_rect(const Rect &r) override;

        bool       set_flags(WindowFlags attr) override;
        bool       set_value(int conf, ...)    override;
        bool       query_value(int what, ...)  override;

        widget_t   bind_widget(widget_t w) override;
        any_t      gc_create(const char_t *type) override;


        LRESULT    proc_keyboard(UINT msg, WPARAM wp, LPARAM lp);

        RECT       adjust_rect(int x, int y, int w, int h);
        DWORD      exstyle() const;
        DWORD      style() const;

        int      btk_to_client(int v) const;
        int      client_to_btk(int v) const;

        POINTS   btk_to_client(POINTS) const;
        POINTS   client_to_btk(POINTS) const;

        HWND         hwnd   = {};
        WindowFlags  flags  = {};
        Win32Driver *driver = {};
        widget_t     widget = {};
        Win32Window *parent = {}; //< For embedding

        bool         erase_background = false;
        bool         mouse_enter = false;
        bool         has_menu  = false;
        Modifier     mod_state = Modifier::None; //< Current keyboard modifiers state
        UINT         win_dpi = 0; //< Window dpi

        HMENU        menu = {}; //< Menu handle
        
        Size         maximum_size = {-1, -1}; //< Maximum window size
        Size         minimum_size = {-1, -1}; //< Minimum window size

        SHORT        mouse_last_x = -1; //< Last mouse position
        SHORT        mouse_last_y = -1; //< Last mouse position

        bool         textinput_enabled = false; //< Text input enabled
        HIMC         imcontext = {}; //< Default IME context

        WINDOWPLACEMENT prev_placement;
};

class Win32Cursor final : public AbstractCursor {
    public:
        Win32Cursor(const PixBuffer &buffer, int hot_x, int hot_y);
        Win32Cursor(SystemCursor system_id);
        ~Win32Cursor();

        void ref();
        void unref();
        void set();
    private:
        HCURSOR cursor = nullptr;
        bool owned = false;
        int  refcount = 1;
};

class Win32Dispatcher final : public EventDispatcher {
    public:
        Win32Dispatcher();
        ~Win32Dispatcher();

        timerid_t timer_add(Object *object, timertype_t type, uint32_t ms) override;
        bool      timer_del(Object *object, timerid_t id) override;

        pointer_t alloc(size_t n) override;
        bool      send(Event *event, EventDtor dtor) override;

        void      interrupt() override;
        int       run()       override;

        BOOL      post_message(UINT msg, WPARAM wp, LPARAM lp);
        VOID      post_quit_message(UINT value);
    private:
        LRESULT   helper_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        using     TimersMap   = std::unordered_map<timerid_t, Win32Timer>;  //< pair<id,  Win32Timer>
        using     StateStack  = std::stack<std::pair<bool, int>>;       //< pair<Running, StatusCode>

        StateStack states      = {};
        TimersMap  timers      = {}; //< Map TimerID to object
        HANDLE     heap        = {}; //< Heap for alloc event
        HWND       helper_hwnd = {}; //< Helper window for timer
    friend class Win32Driver;
};

class Win32Service final : public DesktopService {
    public:
        bool     reparent_native_window(window_t parent, void *native_window, const Rect &placement) override;
        bool     openurl(u8string_view url) override;

};

class Win32Driver final  : public GraphicsDriver, public Win32User32 {
    public:
        Win32Driver();
        ~Win32Driver();

        window_t window_create(u8string_view title, int width, int height, WindowFlags flags) override;
        void     window_add(Win32Window *w);
        void     window_del(Win32Window *w);

        any_t    instance_create(const char_t *what, ...) override;

        void     clipboard_set(u8string_view text) override;
        u8string clipboard_get() override;

        cursor_t cursor_create(const PixBuffer &buf, int hot_x, int hot_y) override;
        cursor_t cursor_create(SystemCursor syscursor) override;

        bool      query_value(int query, ...) override;

        pointer_t service_of(int what) override;

        LRESULT   wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        std::unordered_map<HWND, Win32Window*> windows;

        // Platform data
        Win32Dispatcher dispatcher;
        DWORD           working_thread_id = GetCurrentThreadId();
    private:
        Win32Cursor     syscursor[int(SystemCursor::NumOf)] {
            SystemCursor::Arrow,    
            SystemCursor::Ibeam,    
            SystemCursor::Wait,      
            SystemCursor::Crosshair, 
            SystemCursor::WaitArrow, 
            SystemCursor::SizeNwse,  
            SystemCursor::SizeNesw, 
            SystemCursor::SizeWe,    
            SystemCursor::SizeNs,    
            SystemCursor::SizeAll,   
            SystemCursor::No,        
            SystemCursor::Hand
        };
};

class Win32GLContext final : public GLContext {
    public:
        Win32GLContext(HWND hwnd);
        ~Win32GLContext();

        void begin() override;
        void end() override;
        void swap_buffers() override;

        bool  initialize(const GLFormat &format) override;
        bool  make_current()              override;
        bool  set_swap_interval(int v)    override;
        Size  get_drawable_size() override;
        void *get_proc_address(const char_t *name) override;
    private:
        HDC          hdc    = {};
        HWND         hwnd   = {};
        HGLRC        hglrc  = {};
        Win32Driver *driver = {};
        Win32Window *window = {};

        HDC          prev_dc = {};
        HGLRC        prev_rc = {};

        // OpenGL functions pointers
        GL_LIB      ("opengl32.dll");
        GL_PROC     (wglMakeCurrent);
        GL_PROC     (wglCreateContext);
        GL_PROC     (wglDeleteContext);
        GL_PROC     (wglGetProcAddress);
        GL_PROC     (wglGetCurrentDC);
        GL_PROC     (wglGetCurrentContext);
        GL_PROC     (wglShareLists);

        // WGL Extension functions pointers
        GL_WPROC    (wglChoosePixelFormatARB);
        GL_WPROC    (wglGetExtensionsStringARB);
        GL_WPROC    (wglSetSwapIntervalEXT);

};

// For invoke a call in GUI Thread and wait the result
template <typename Callable, typename ...Args>
class Win32Call : public std::tuple<Args...> {
    public:
        using RetT = typename std::invoke_result_t<Callable, Args...>;
        
        Win32Call(Callable &&c, Args &&...args) :
            std::tuple<Args...>(std::forward<Args>(args)...),
            callable(std::forward<Callable>(c))
        {
            sync = CreateEventW(nullptr, false, false, nullptr);
            assert(sync != INVALID_HANDLE_VALUE);
        }


        RetT result() const {
            WaitForSingleObject(sync, INFINITE);
            if constexpr (std::is_same_v<RetT, void>) {
                return ;
            }
            else {
                return ret;
            }
        }

        static void Call(void *self) {
            auto *call = static_cast<Win32Call*>(self);

            // Check return type is void and call it
            if constexpr (std::is_same_v<RetT, void>) {
                std::apply(call->callable, static_cast<std::tuple<Args...>&&>(*call));
            }
            else {
                call->ret = std::apply(call->callable, static_cast<std::tuple<Args...>&&>(*call));
            }
            // Set the signal
            SetEvent(call->sync);
        }
    private:
        std::conditional_t<
            std::is_same_v<RetT, void>,
            int,
            RetT
        > ret;
        HANDLE   sync;
        Callable callable;
};

// Global instance of the driver.
static Win32Driver     *win32_driver = nullptr;
static Win32Dispatcher *win32_dispatcher = nullptr;

// Helper function 
size_t utf8_to_utf16(const char_t *utf8, size_t u8len, wchar_t *utf16, size_t u16len) {
    size_t ret = ::MultiByteToWideChar(CP_UTF8, 0, utf8, u8len, utf16, u16len);
    utf16[ret] = 0; // Let output be null terminated.
    return ret;
}
template <typename Callable, typename ...Args>
auto   gui_thread_call(Callable &&c, Args &&...args) {
    Win32Call<Callable, Args...> call(
        std::forward<Callable>(c),
        std::forward<Args>(args)...
    );

    // Post the message to the helper window.
    win32_dispatcher->post_message(
        BTK_CALL,
        reinterpret_cast<WPARAM>(Win32Call<Callable, Args...>::Call),
        reinterpret_cast<LPARAM>(&call)
    );

    return call.result();
}
HBITMAP buffer_to_hbitmap(const PixBuffer &buf) {
    // We need to convert to BGRA
    int w = buf.width();
    int h = buf.height();
    uint32_t *dst = static_cast<uint32_t*>(_malloca(w * h * 4));

    if (buf.format() == PixFormat::RGBA32) {
        auto src = static_cast<const uint32_t*>(buf.pixels());
        for (int i = 0; i < w * h; i++) {
            dst[i] = (src[i] & 0xFF00FF00) | ((src[i] & 0xFF) << 16) | ((src[i] & 0xFF0000) >> 16);
        }
    }
    else if (buf.format() == PixFormat::RGB24) {
        // 3 bytes per pixel, no alpha
        auto src = static_cast<const uint8_t*>(buf.pixels());
        for (int i = 0; i < w * h; i++) {
            dst[i] = (src[i * 3 + 2] << 16) | (src[i * 3 + 1] << 8) | src[i * 3];
        }
    }
    else {
        // Unsupport now ! just zero buffer
        Btk_memset(dst, 0, w * h * 4);
    }

    HBITMAP hbitmap = CreateBitmap(buf.width(), buf.height(), 1, 32, dst);
    _freea(dst);
    return hbitmap;
}

Win32Dispatcher::Win32Dispatcher() {
    SetDispatcher(this);

    // Init Heap
    heap = HeapCreate(0, 0, 0);
    HeapSetInformation(
        heap,
        HeapEnableTerminationOnCorruption,
        nullptr,
        0
    );
    assert(heap != nullptr);

    // Initialize helper window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = BTK_HELPER_CLASS;
    wc.lpfnWndProc = [](HWND w, UINT m, WPARAM wp, LPARAM lp) {
        return win32_dispatcher->helper_wnd_proc(w, m, wp, lp);
    };

    // Reg it
    RegisterClassExW(&wc);

    // Create Helper window
    helper_hwnd = CreateWindowExW(
        0,
        BTK_HELPER_CLASS,
        nullptr,
        0,
        0,
        0,
        32,
        32,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    assert(helper_hwnd != nullptr);

    win32_dispatcher = this;
}
Win32Dispatcher::~Win32Dispatcher() {
    if (GetDispatcher() == this) {
        SetDispatcher(nullptr);
    }
    DestroyWindow(helper_hwnd);
    HeapDestroy(heap);

    win32_dispatcher = nullptr;
}
timerid_t Win32Dispatcher::timer_add(Object *object, timertype_t t, uint32_t ms) {
    timerid_t id;
    timerid_t offset = 0;
    HANDLE    handle = INVALID_HANDLE_VALUE; //< System timer handle
    if (t == TimerType::Precise) {
        offset = TIMER_PRECISE;
    }
    // Random a id
    do {
        id = rand();
        id %= TIMER_PRECISE; //< Avoid overflow
        id += offset;//< Add offset if is precise timer
    }
    while(timers.find(id) != timers.end());

    if (!offset) {
        // Coarse timer
        // Let system add it
        id = SetTimer(
            helper_hwnd, 
            id,
            ms,
            nullptr
        );

        if (id == 0) {
            WIN_LOG("SetTimer failed\n");
            return 0;
        }
    }
    else {
        // Use winapi
        auto cb = [](void *id, BOOLEAN) -> void {
            win32_dispatcher->post_message(
                WM_TIMER,
                reinterpret_cast<WPARAM>(id),
                0
            );
        };
        if (!CreateTimerQueueTimer(&handle, nullptr, cb, reinterpret_cast<void*>(id), ms, ms, WT_EXECUTEDEFAULT)) {
            // Unsupported ?
            WIN_LOG("CreateTimerQueueTimer failed");
            // Back to coarse timer
            return timer_add(object, timertype_t::Coarse, ms);
        }
    }

    // Register to timer map
    timers[id].handle = handle;
    timers[id].object = object;
    return id;
}
bool      Win32Dispatcher::timer_del(Object *object, timerid_t id) {
    bool ret = false;
    auto iter = timers.find(id);
    if (iter == timers.end()) {
        return false;
    }

    // Got
    if (TIMER_IS_COARSE(id)) {
        // Timer from SetTimer
        ret = KillTimer(helper_hwnd, id);
    }
    else {
        // Timer from CreateTimerQueueTimer
        ret = DeleteTimerQueueTimer(nullptr, iter->second.handle, nullptr);
        assert(ret);
    }

    timers.erase(iter);

    return ret;
}
LRESULT   Win32Dispatcher::helper_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_TIMER : {
            // Check if is timer event , and it is on the list of timers.
            if (timers.find(wparam) != timers.end()) {

                auto id     = wparam;
                auto object = timers[id].object;

                TimerEvent event(object, id);
                event.set_timestamp(GetTicks());

                object->handle(event);
                return 0;
            }
            else {
                //??? 
                WIN_LOG("Unregitered WM_TIMER id => %d\n", int(wparam));
            }
            return 0;
        }
        case BTK_CALL : {
            auto callback = reinterpret_cast<Win32Callback>(wparam);
            auto param    = reinterpret_cast<void *>(lparam);
            callback(param);
            return 0;
        }
        case BTK_EVENT : {
            auto eventptr  = reinterpret_cast<Event*>(wparam);
            auto eventdtor = reinterpret_cast<EventDtor>(lparam);

            dispatch(eventptr);

            if (eventdtor) {
                eventdtor(eventptr);
            }

            // Btk_free(eventptr);
            if (!HeapFree(heap, 0, eventptr)) {
                WIN_LOG("[Win32Dispatcher] HeapFree %p failed\n", eventptr);
            }
            return 0;
        }
        default : {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }
}
pointer_t Win32Dispatcher::alloc(size_t n) {
    // return Btk_malloc(n);
    return HeapAlloc(heap, 0, n);
}
bool      Win32Dispatcher::send(Event *event, EventDtor dtor) {
    return post_message(
        BTK_EVENT,
        reinterpret_cast<WPARAM>(event),
        reinterpret_cast<LPARAM>(dtor)
    );
}
int       Win32Dispatcher::run() {
    // Push Kepp <Running, Normal exit code>
    states.push({true, EXIT_SUCCESS});
    
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) && states.top().first) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (msg.message == WM_QUIT) {
        states.top().second = msg.wParam;
    }
    int code = states.top().second;

    // Restore
    states.pop();
    return code;
}
void    Win32Dispatcher::interrupt() {
    if (!states.empty()) {
        states.top().first = true;
    }
}
BOOL    Win32Dispatcher::post_message(UINT msg, WPARAM wp, LPARAM lp) {
    return PostMessageW(
        helper_hwnd,
        msg,
        wp,
        lp
    );
}


Win32Driver::Win32Driver() {
    // Set DPI if avliable
    if (SetProcessDpiAwarenessContext) {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
    // Initialize the window class.
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND w, UINT m, WPARAM wp, LPARAM lp) {
        return win32_driver->wnd_proc(w, m, wp, lp);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    // msdn says system will set the cursor when mouse move
    // wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = BTK_WINDOW_CLASS;
    wc.hbrBackground = (HBRUSH)0;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Initialize the IME

    // Set the instance
    win32_driver = this;

    // Set the fallback
    if (!GetDpiForWindow) {
        GetDpiForWindow = [](HWND h) -> UINT {
            HDC dc = GetDC(h);
            UINT dpi = GetDeviceCaps(dc, LOGPIXELSX);
            ReleaseDC(h, dc);
            return dpi;
        };
    }
    if (!GetDpiForSystem) {
        GetDpiForSystem = []() -> UINT {
            return win32_driver->GetDpiForWindow(GetDesktopWindow());
        };
    }
    if (!AdjustWindowRectExForDpi) {
        AdjustWindowRectExForDpi = [](LPRECT rect, DWORD style, BOOL has_menu, DWORD exstyle, UINT dpi) -> BOOL {
            if (!AdjustWindowRectEx(rect, style, has_menu, exstyle)) {
                return FALSE;
            }
            // Scale by dpi
            UINT width = rect->right - rect->left;
            UINT height = rect->bottom - rect->top;

            width = MulDiv(width, dpi, 96);
            height = MulDiv(height, dpi, 96);

            rect->right = rect->left + width;
            rect->bottom = rect->top + height;

            return TRUE;
        };
    }

    Win32BacktraceInit();
}
Win32Driver::~Win32Driver() {
    win32_driver = nullptr;
}


LRESULT Win32Driver::wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // First Get window from the map.
    auto it = windows.find(hwnd);
    if (it == windows.end()) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    if (it->second->widget == nullptr) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    Win32Window *win = it->second;
    Widget *widget   = win->widget;
    switch (msg) {
        // case WM_QUIT : {
        //     context->send_event(QuitEvent(wparam));
        //     break;
        // }
        case WM_CLOSE : {
            CloseEvent event;
            event.accept(); //< Default is to accept the close event.
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            
            widget->handle(event);

            if (event.is_accepted()) {
                // Remove from the list of windows.
                // TODO : What should we do next? Destroy the window?
                windows.erase(it);
                ShowWindow(hwnd, SW_HIDE);

                if (windows.empty()) {
                    WIN_LOG("[Win32] All window closed, Quitting\n");
                    PostQuitMessage(0);
                    
                    // Event e(Event::Quit);
                    // context->send_event(e);
                }
            }
            break;
        }
        case WM_PAINT : {
            PaintEvent event;
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            widget->handle(event);
            // Draw children
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_ERASEBKGND : {
            // Default diaable
            // For avoid flickering, we disable the background erase.
            // WIN_LOG("WM_ERASEBKGND\n");
            if (win->erase_background) {
                return DefWindowProcW(hwnd, msg, wparam, lparam);
            }
            return 1;
        }
        case WM_SIZE : {
            ResizeEvent event;
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            auto [w, h] = win->size();

            if (w == 0 || h == 0) {
                // Probably mini, ignore it
                break;
            }

            WIN_LOG("[Win32] Window resize to Size(%d, %d)\n", w, h);

            event.set_new_size(
                // LOWORD(lparam), HIWORD(lparam)
                w, h
            );

            widget->handle(event);
            break;
        }
        case WM_GETMINMAXINFO : {
            MINMAXINFO *mmi = (MINMAXINFO*)lparam;
            if (win->maximum_size.is_valid()) {
                auto rect = win->adjust_rect(
                    0, 
                    0, 
                    win->btk_to_client(win->maximum_size.w), 
                    win->btk_to_client(win->maximum_size.h)
                );
                mmi->ptMaxTrackSize.x = rect.right - rect.left;
                mmi->ptMaxTrackSize.y = rect.bottom - rect.top;
            }
            if (win->minimum_size.is_valid()) {
                auto rect = win->adjust_rect(
                    0, 
                    0, 
                    win->btk_to_client(win->minimum_size.w), 
                    win->btk_to_client(win->minimum_size.h)
                );
                mmi->ptMinTrackSize.x = rect.right - rect.left;
                mmi->ptMinTrackSize.y = rect.bottom - rect.top;
            }
            break;
        }
        // case WM_COMMAND : {
        //     printf("Menu command %d\n", wparam);
        //     break;
        // }
        case WM_SHOWWINDOW : {
            Event::Type type;
            if (wparam) {
                type = Event::Show;
            }
            else {
                type = Event::Hide;
            }
            WidgetEvent event(type);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_MOUSEMOVE : {
            if (!win->mouse_enter) {
                // WIN_LOG("Mouse enter\n");
                win->mouse_enter = true;
                WidgetEvent event(Event::MouseEnter);
                event.set_widget(win->widget);
                event.set_timestamp(GetTicks());
                widget->handle(event);

                // Track mouse leave
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }

            auto point = win->client_to_btk(MAKEPOINTS(lparam));

            MotionEvent event(point.x, point.y);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            if (win->mouse_last_x != -1 && win->mouse_last_y != -1) {
                event.set_rel(
                    win->client_to_btk(point.x - win->mouse_last_x),
                    win->client_to_btk(point.y - win->mouse_last_y)
                );
            }

            // Update previous position
            win->mouse_last_x = point.x;
            win->mouse_last_y = point.y;

            widget->handle(event);
            break;
        }
        case WM_MOUSELEAVE : {
            // WIN_LOG("Mouse leave\n");
            win->mouse_enter = false;
            win->mouse_last_x = -1;
            win->mouse_last_y = -1;

            WidgetEvent event(Event::MouseLeave);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);

            // Untrack mouse leave
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE | TME_CANCEL;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);

            break;
        }
        case WM_MOUSEWHEEL : {
            [[fallthrough]];
        }
        case WM_MOUSEHWHEEL : {
            int x = 0;
            int y = 0;

            if (msg == WM_MOUSEWHEEL) {
                // WIN_LOG("WM_MOUSEWHEEL\n");
                y = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
            }
            else {
                // WIN_LOG("WM_HMOUSEWHEEL\n");
                x = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
            }

            WIN_LOG("[Win32] Mouse wheel %d, %d\n", x, y);

            WheelEvent event(x, y);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_LBUTTONDOWN : {
            [[fallthrough]];
        }
        case WM_RBUTTONDOWN : {
            auto point = win->client_to_btk(MAKEPOINTS(lparam));
            MouseEvent event(
                Event::MousePress,
                point.x, point.y,
                wparam
            );
            if (msg == WM_LBUTTONDOWN) {
                event.set_button(MouseButton::Left);
            }
            else {
                event.set_button(MouseButton::Right);
            }
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_LBUTTONUP : {
            [[fallthrough]];
        }
        case WM_RBUTTONUP : {
            auto pos = win->client_to_btk(MAKEPOINTS(lparam));
            MouseEvent event(Event::MouseRelease, pos.x, pos.y, wparam);
            if (msg == WM_LBUTTONUP) {
                event.set_button(MouseButton::Left);
            }
            else {
                event.set_button(MouseButton::Right);
            }
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_SYSKEYDOWN : {
            [[fallthrough]];
        }
        case WM_SYSKEYUP : {
            [[fallthrough]];
        }
        case WM_KEYDOWN : {
            [[fallthrough]];
        }
        case WM_KEYUP : {
            // Forward to widget
            return win->proc_keyboard(msg, wparam, lparam);
        }
        case WM_SETFOCUS : {
            WidgetEvent event(Event::FocusGained);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_KILLFOCUS : {
            WidgetEvent event(Event::FocusLost);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_DROPFILES : {
            auto drop = (HDROP)wparam;

            // Get where the drop happened
            POINT p;            
            DragQueryPoint(drop, &p);
            p.x = win->client_to_btk(p.x);
            p.y = win->client_to_btk(p.y);

            // Get drop string
            UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            WIN_LOG("[Win32] Drop count: %d\n", count);

            // Begin drop
            DropEvent event;
            event.set_timestamp(GetTicks());
            event.set_position(p.x, p.y);
            event.set_widget(win->widget);
            event.set_type(Event::DropBegin);

            widget->handle(event);

            std::wstring drop_string;
            for (UINT i = 0; i < count; i++) {
                // Query the file name
                UINT len = DragQueryFileW(drop, i, nullptr, 0);
                drop_string.reserve(len);

                DragQueryFileW(drop, i, drop_string.data(), len);

                // Make DropEvent
                event.set_text(u8string::from(drop_string));
                event.set_type(Event::DropFile);

                widget->handle(event);
            }

            // End drop
            event.set_type(Event::DropEnd);
            widget->handle(event);

            DragFinish(drop);
            break;
        }
        // IME Here
        case WM_IME_CHAR : {
            // WIN_LOG("WM_IME_CHAR char: %d\n", wparam);
            if (!win->textinput_enabled) {
                break;
            }

            uchar_t c = wparam;
            TextInputEvent event(u8string::from(&c, 1));
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_CHAR : {
            // if (!win->textinput_enabled) {
            //     WIN_LOG("Text input disabled, Drop it\n");
            //     break;
            // }
            // Check the range of the char
            if (!win->textinput_enabled) {
                // WIN_LOG("TextInput disabled, Drop it\n");
                break;
            }
            if (wparam < 0x20 || wparam > 0x7E) {
                // WIN_LOG("Drop char: %d\n", wparam);
                break;
            }
            else {
                // WIN_LOG("WM_CHAR : %d\n", wparam);
            }

            char16_t c = wparam;
            TextInputEvent event(u8string::from(&c, 1));
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_DPICHANGED : {
            RECT *suggested_rect = reinterpret_cast<RECT*>(lparam);
            win->win_dpi = HIWORD(wparam);

            WIN_LOG("[Win32] DPI Changed to %d\n", int(win->win_dpi));
            break;
        }

        default : {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }
    return 0;
}
window_t Win32Driver::window_create(u8string_view title, int width, int height, WindowFlags flags) {
    if (working_thread_id != GetCurrentThreadId()) {
        return gui_thread_call([&, this](){
            return window_create(title, width, height, flags);
        });
    }


    // Alloc tmp space for the title.
    auto u16 = title.to_utf16();
    auto wtitle = reinterpret_cast<const wchar_t*>(u16.c_str());

    // Check args if
    DWORD style = WS_OVERLAPPEDWINDOW;
    UINT  dpi   = GetDpiForSystem();

    // Parse flags

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.bottom = height;
    rect.right = width;
    if (!AdjustWindowRectExForDpi(&rect, style, FALSE, 0, dpi)) {
        WIN_LOG("[Win32::Error] Bad adjust");
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    WIN_LOG("[Win32] Creating window %d %d\n", width, height);


    HWND hwnd = CreateWindowExW(
        0,
        BTK_WINDOW_CLASS,
        wtitle,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (!hwnd) {
        return nullptr;
    }

    // Configure window
    DragAcceptFiles(hwnd, TRUE);

    auto win = new Win32Window(hwnd, this);
    win->win_dpi   = dpi;
    win->flags     = flags;

    // Check flags
    if ((flags & WindowFlags::NeverShowed) != WindowFlags::NeverShowed) {
        window_add(win);
    }
    return win;
}
void  Win32Driver::window_add(Win32Window *win) {
    windows[win->hwnd] = win;
}
void  Win32Driver::window_del(Win32Window *win) {
    windows.erase(win->hwnd);
}
void  Win32Driver::clipboard_set(u8string_view text) {
    if (!OpenClipboard(win32_dispatcher->helper_hwnd)) {
        return;
    }
    size_t len = text.size();
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (len + 1));
    if (!hmem) {
        return;
    }
    wchar_t *mem = (wchar_t *)GlobalLock(hmem);
    utf8_to_utf16(text.data(), len, mem, len + 1);
    GlobalUnlock(hmem);
    SetClipboardData(CF_UNICODETEXT, hmem);

    CloseClipboard();
}
u8string Win32Driver::clipboard_get() {
    u8string text;
    if (OpenClipboard(win32_dispatcher->helper_hwnd)) {
        HANDLE h = GetClipboardData(CF_UNICODETEXT);
        if (h) {
            wchar_t *wtext = (wchar_t *)GlobalLock(h);
            if (wtext) {
                text = u8string::from(wtext);
            }
        }
        CloseClipboard();
    }
    return text;
}
bool Win32Driver::query_value(int query, ...) {
    va_list varg;
    va_start(varg, query);
    bool ret = true;

    switch (query) {
        case SystemDpi : {
            auto dpi = va_arg(varg, FPoint*);
            dpi->x = GetDpiForSystem();
            dpi->y = GetDpiForSystem();
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
any_t Win32Driver::instance_create(const char_t *what, ...) {
    va_list varg;
    va_start(varg, what);
    any_t ret = nullptr;

    if (_stricmp(what, "filedialog") == 0) {
        // Etc...
        return new Win32FileDialog;
    }

    va_end(varg);
    return ret;
}
cursor_t Win32Driver::cursor_create(const PixBuffer &buf, int hot_x, int hot_y) {
    return new Win32Cursor(buf, hot_x, hot_y);
}
cursor_t Win32Driver::cursor_create(SystemCursor sys) {
    return &syscursor[int(sys)];
}
pointer_t Win32Driver::service_of(int what) {
    return nullptr;
}

Win32Window::Win32Window(HWND h, Win32Driver *d) {
    hwnd = h;
    driver = d;

    // Default disable IME
    imcontext = ImmAssociateContext(hwnd, nullptr);
}
Win32Window::~Win32Window() {
    driver->window_del(this);

    DestroyWindow(hwnd);
}

Size Win32Window::size() const {
    RECT r;
    GetClientRect(hwnd, &r);
    return Size(
        client_to_btk(r.right - r.left), 
        client_to_btk(r.bottom - r.top)
    );
}
Point Win32Window::position() const {
    RECT r;
    GetWindowRect(hwnd, &r);
    return Point(
        r.left, r.top
    );
}
void Win32Window::raise() {
    SetForegroundWindow(hwnd);
}
void Win32Window::close() {
    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    // CloseWindow(hwnd);
}
void Win32Window::repaint() {
#if 1
    InvalidateRect(hwnd, nullptr, FALSE);
#else
    // Our paint event
    DWORD64 now = GetTickCount64();
    DWORD64 limit = 1000 / repaint_limit;
    if (now - last_repaint < limit && !repaint_in_progress) {
        return;
    }
    repaint_in_progress = true;
    last_repaint = now;

    PostMessageW(hwnd, WM_REPAINT, 0, 0);
#endif
}
// TODO : Support DPI Scale
void Win32Window::resize(int w, int h) {
    w = btk_to_client(w);
    h = btk_to_client(h);

    auto [x, y] = position();
    auto rect   = adjust_rect(0, 0, w, h);

    MoveWindow(
        hwnd,
        x,
        y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        TRUE
    );
}
void Win32Window::move(int x, int y) {
    auto [w, h] = size();
    auto rect   = adjust_rect(x, y, w, h);

    MoveWindow(
        hwnd,
        rect.left,
        rect.top,
        w,
        h,
        FALSE
    );
}
void Win32Window::show(int v) {
    if (v) {
        // Check if the window is on device.
        if (driver->windows.find(hwnd) == driver->windows.end()) {
            driver->window_add(this);
        }
    }
    int cmd;
    switch (v) {
        case Hide : cmd = SW_HIDE; break;
        case Show : cmd = SW_SHOW; break;
        case Restore  : cmd = SW_RESTORE; break;
        case Maximize : cmd = SW_MAXIMIZE; break;
        case Minimize : cmd = SW_MINIMIZE; break;

        default : WIN_LOG("[Win32::Error] Bad Show flag") cmd = SW_SHOW; break;
    }

    ShowWindow(hwnd, cmd);
}
void Win32Window::set_title(u8string_view title) {
    auto u16 = title.to_utf16();
    auto wtitle = reinterpret_cast<const wchar_t*>(u16.c_str());
    SetWindowTextW(hwnd, wtitle);
}
void Win32Window::set_icon(const PixBuffer &pixbuf) {
    // Get pixbuf format first
    if (pixbuf.format() == PixFormat::RGBA32) {
        ICONINFO info = {};
        info.fIcon    = TRUE;
        info.hbmColor = buffer_to_hbitmap(pixbuf);
        info.hbmMask  = CreateBitmap(pixbuf.width(), pixbuf.height(), 1, 1, nullptr);

        auto icon = CreateIconIndirect(&info);

        DeleteObject(info.hbmColor);
        DeleteObject(info.hbmMask);
        if (icon) {
            // Set icon
            SendMessageW(hwnd, WM_SETICON, ICON_BIG,    (LPARAM)icon);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL,  (LPARAM)icon);

#if         defined(ICON_SMALL2)
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL2, (LPARAM)icon);
#endif
        }
        else {
            WIN_LOG("[Win32::Error] CreateIconIndirect failed\n");
            WIN_LOG("[Win32::Error] GetLastError: %d\n", GetLastError());
        }
    }
}
widget_t Win32Window::bind_widget(widget_t w) {
    auto prev = widget;
    widget = w;
    return prev;
}

void     Win32Window::capture_mouse(bool v) {
    if (v) {
        SetCapture(hwnd);
    }
    else {
        ReleaseCapture();
    }
}
bool     Win32Window::set_flags(WindowFlags new_flags) {
    auto bit_changed = [=](WindowFlags what) {
        return (flags & what) != (new_flags & what);
    };

    int err = 0;

    if (bit_changed(WindowFlags::AcceptDrop)) {
        DragAcceptFiles(hwnd, bool(new_flags & WindowFlags::AcceptDrop));
    }
    if (bit_changed(WindowFlags::Resizable)) {
        DWORD style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (bool(new_flags & WindowFlags::Resizable)) {
            style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
        }
        else {
            style ^= (WS_SIZEBOX | WS_MAXIMIZEBOX);
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    }
    if (bit_changed(WindowFlags::Fullscreen)) {
        // Using the way of https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
        bool value = bool(new_flags & WindowFlags::Fullscreen);
        if (value) {
            MONITORINFO info;
            info.cbSize = sizeof(info);

            if (
                GetWindowPlacement(hwnd, &prev_placement) && 
                GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &info)
            ) {
                // Save style
                auto win_style   = style();

                // Change style
                win_style ^= WS_OVERLAPPEDWINDOW;
                SetWindowLongPtrW(hwnd, GWL_STYLE, win_style);
                SetWindowPos(
                    hwnd,
                    HWND_TOPMOST,
                    info.rcMonitor.left,
                    info.rcMonitor.top,
                    info.rcMonitor.right  - info.rcMonitor.left,
                    info.rcMonitor.bottom - info.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                );
            }
            else {
                // Err
                WIN_LOG("[Win32::Error] Failed to SetFullscreen\n");
                err = 1;
            }
        }
        else {
            // Reset back
            auto win_style = style();
            win_style |= WS_OVERLAPPEDWINDOW;
            SetWindowLongPtrW(hwnd, GWL_STYLE, win_style);
            SetWindowPlacement(hwnd, &prev_placement);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
    }
    // if (bit_changed(WindowFlags::Transparent)) {
    //     DWORD new_ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    //     if (bool(new_flags & WindowFlags::Transparent)) {
    //         new_ex_style |= WS_EX_LAYERED;
    //     }
    //     else {
    //         new_ex_style ^= WS_EX_LAYERED;
    //     }

    //     SetWindowLongPtrW(hwnd, GWL_EXSTYLE, new_ex_style);
    //     win_ex_style = new_ex_style;

    //     if (bool(new_flags & WindowFlags::Transparent)) {
    //         SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 1, LWA_COLORKEY);
    //     }
    //     // TODO :
    // }
    flags = new_flags;
    return err == 0;
}
bool     Win32Window::set_value(int conf, ...) {
    va_list varg;
    va_start(varg, conf);
    bool ret = true;

    switch (conf) {
        case MinimumSize : {
            minimum_size = *va_arg(varg, Size*);
            break;
        }
        case MaximumSize : {
            maximum_size = *va_arg(varg, Size*);
            break;
        }
        case MousePosition : {
            auto pos = *va_arg(varg, Point*);
            POINT p;
            p.x = btk_to_client(pos.x);
            p.y = btk_to_client(pos.y);
            ret = ClientToScreen(hwnd, &p);
            if (ret) {
                ret = SetCursorPos(p.x, p.y);
            }
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
bool     Win32Window::query_value(int query, ...) {
    va_list varg;
    va_start(varg, query);
    bool ret = true;

    switch (query) {
        case NativeHandle : {
            [[fallthrough]];
        }
        case Hwnd : {
            *va_arg(varg, HWND*) = hwnd;
            break;
        }
        case Dpi : {
            auto dpi = driver->GetDpiForWindow(hwnd);
            auto pp = va_arg(varg, FPoint*);
            pp->x = dpi;
            pp->y = dpi;
            break;
        }
        case MousePosition : {
            POINT pos;
            if (!GetCursorPos(&pos)) {
                ret = false;
                break;
            }
            if (!ScreenToClient(hwnd, &pos)) {
                ret = false;
                break;
            }
            Point p;
            p.x = client_to_btk(pos.x);
            p.y = client_to_btk(pos.y);

            *va_arg(varg, Point*) = p;
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
void     Win32Window::set_textinput_rect(const Rect &rect) {
    auto imc = ImmGetContext(hwnd);
    if (imc) {
        COMPOSITIONFORM cf = {};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = btk_to_client(rect.x);
        cf.ptCurrentPos.y = btk_to_client(rect.y + rect.h);
        ImmSetCompositionWindow(imc, &cf);

        ImmReleaseContext(hwnd, imc);
    }
}
void     Win32Window::start_textinput(bool v) {
    textinput_enabled = v;

    if (v) {
        // Enable ime
        ImmAssociateContext(hwnd, imcontext);
    }
    else {
        // Disable ime
        ImmAssociateContext(hwnd, nullptr);
    }
}
any_t     Win32Window::gc_create(const char_t *name) {
    if (_stricmp(name, "opengl") == 0) {
        if (SetWindowLongPtrW(hwnd, GWL_STYLE, GetWindowLongPtrW(hwnd, GWL_STYLE) | CS_OWNDC) == 0) {
            // Failed
            return nullptr;
        }
        return new Win32GLContext(hwnd);
    }
    return nullptr;
}

LRESULT Win32Window::proc_keyboard(UINT msg, WPARAM wp, LPARAM lp) {
    Event::Type type;
    bool press;
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        type = Event::KeyPress;
        press = true;
    }
    else {
        type = Event::KeyRelease;
        press = false;
    }

    auto proc_mod = [this, press](Modifier m) {
        if (press) {
            mod_state |= m;
        }
        else {
            mod_state &= ~m;
        }
        WIN_LOG("[Win32::Keyboard] mod_state: %x\n", mod_state);
    };

    WORD vk = LOWORD(wp); //< Virtual key code
    WORD key_flags = HIWORD(lp); //< Key flags
    WORD scancode  = LOBYTE(key_flags); //< Get scancode for distinguish mod

    bool is_extended_key = (key_flags & KF_EXTENDED) == KF_EXTENDED;
    bool has_alt = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;
    WORD repeat_count = LOWORD(lp); //< Repeat count

    Key  keycode = Key::Unknown;

    // I don't know why, MSDN say we should do it
    if (is_extended_key) {
        scancode = MAKEWORD(scancode, 0xE0);
    }

    // Process virtual key code 
    // TODO : Still need improve
    switch(vk) {
        // Modifier keys begin
        case VK_SHIFT : {   // => VK_RSHIFT / VK_LSHIFT
            if (is_extended_key) {
                WIN_LOG ("[Win32::Keyboard] VK_RSHIFT\n");
                proc_mod(Modifier::Rshift);
            }
            else {
                WIN_LOG ("[Win32::Keyboard] VK_LSHIFT\n");
                proc_mod(Modifier::Lshift);
            }
            break;
        }
        case VK_CONTROL : {
            if (is_extended_key) {
                WIN_LOG ("[Win32::Keyboard] VK_RCONTROL\n");
                proc_mod(Modifier::Rctrl);
            }
            else {
                WIN_LOG ("[Win32::Keyboard] VK_LCONTROL\n");
                proc_mod(Modifier::Lctrl);
            }
            break;
        }
        case VK_MENU : {
            if (is_extended_key) {
                WIN_LOG ("[Win32::Keyboard] VK_RMENU\n");
                proc_mod(Modifier::Ralt);
            }
            else {
                WIN_LOG ("[Win32::Keyboard] VK_LMENU\n");
                proc_mod(Modifier::Lalt);
            }
            break;
        }

        // Normal key
        MAP_KEY(VK_BACK, Key::Backspace)
        MAP_KEY(VK_TAB, Key::Tab)
        MAP_KEY(VK_CLEAR, Key::Clear)
        MAP_KEY(VK_RETURN, Key::Return)
        MAP_KEY(VK_ESCAPE, Key::Escape)
        MAP_KEY(VK_SPACE, Key::Space)
        MAP_KEY(VK_PRIOR, Key::Pageup)
        MAP_KEY(VK_NEXT, Key::Pagedown)
        MAP_KEY(VK_END, Key::End)
        MAP_KEY(VK_HOME, Key::Home)
        MAP_KEY(VK_LEFT, Key::Left)
        MAP_KEY(VK_UP, Key::Up)
        MAP_KEY(VK_RIGHT, Key::Right)
        MAP_KEY(VK_DOWN, Key::Down)
        MAP_KEY(VK_SELECT, Key::Select)
        // MAP_KEY(VK_PRINT, Key::Print) // What is this?
        MAP_KEY(VK_EXECUTE, Key::Execute)
        MAP_KEY(VK_SNAPSHOT, Key::Printscreen)
        MAP_KEY(VK_INSERT, Key::Insert)
        MAP_KEY(VK_DELETE, Key::Delete)
        MAP_KEY(VK_HELP, Key::Help)

        // From 0x30 to 0x39 are digits // TODO : Check shift to translate
        MAP_KEY(0x30, Key::_0)
        MAP_KEY(0x31, Key::_1)
        MAP_KEY(0x32, Key::_2)
        MAP_KEY(0x33, Key::_3)
        MAP_KEY(0x34, Key::_4)
        MAP_KEY(0x35, Key::_5)
        MAP_KEY(0x36, Key::_6)
        MAP_KEY(0x37, Key::_7)
        MAP_KEY(0x38, Key::_8)
        MAP_KEY(0x39, Key::_9)

        // From 0x41 to 0x5A are letters
        MAP_KEY(0x41, Key::A)
        MAP_KEY(0x42, Key::B)
        MAP_KEY(0x43, Key::C)
        MAP_KEY(0x44, Key::D)
        MAP_KEY(0x45, Key::E)
        MAP_KEY(0x46, Key::F)
        MAP_KEY(0x47, Key::G)
        MAP_KEY(0x48, Key::H)
        MAP_KEY(0x49, Key::I)
        MAP_KEY(0x4A, Key::J)
        MAP_KEY(0x4B, Key::K)
        MAP_KEY(0x4C, Key::L)
        MAP_KEY(0x4D, Key::M)
        MAP_KEY(0x4E, Key::N)
        MAP_KEY(0x4F, Key::O)
        MAP_KEY(0x50, Key::P)
        MAP_KEY(0x51, Key::Q)
        MAP_KEY(0x52, Key::R)
        MAP_KEY(0x53, Key::S)
        MAP_KEY(0x54, Key::T)
        MAP_KEY(0x55, Key::U)
        MAP_KEY(0x56, Key::V)
        MAP_KEY(0x57, Key::W)
        MAP_KEY(0x58, Key::X)
        MAP_KEY(0x59, Key::Y)
        MAP_KEY(0x5A, Key::Z)

        // RGui / LGui
        MAP_KEY(VK_LWIN, Key::Lgui)
        MAP_KEY(VK_RWIN, Key::Rgui)
        MAP_KEY(VK_APPS, Key::Application)

        MAP_KEY(VK_SLEEP, Key::Sleep)

        // Numpad
        MAP_KEY(VK_NUMPAD0, Key::Kp_0)
        MAP_KEY(VK_NUMPAD1, Key::Kp_1)
        MAP_KEY(VK_NUMPAD2, Key::Kp_2)
        MAP_KEY(VK_NUMPAD3, Key::Kp_3)
        MAP_KEY(VK_NUMPAD4, Key::Kp_4)
        MAP_KEY(VK_NUMPAD5, Key::Kp_5)
        MAP_KEY(VK_NUMPAD6, Key::Kp_6)
        MAP_KEY(VK_NUMPAD7, Key::Kp_7)
        MAP_KEY(VK_NUMPAD8, Key::Kp_8)
        MAP_KEY(VK_NUMPAD9, Key::Kp_9)

        MAP_KEY(VK_MULTIPLY, Key::Kp_Multiply)
        MAP_KEY(VK_ADD, Key::Kp_Plus)
        MAP_KEY(VK_SEPARATOR, Key::Separator)
        MAP_KEY(VK_SUBTRACT, Key::Kp_Minus)
        MAP_KEY(VK_DECIMAL, Key::Kp_Decimal)
        MAP_KEY(VK_DIVIDE, Key::Kp_Divide)

        // F1 to F12
        MAP_KEY(VK_F1, Key::F1)
        MAP_KEY(VK_F2, Key::F2)
        MAP_KEY(VK_F3, Key::F3)
        MAP_KEY(VK_F4, Key::F4)
        MAP_KEY(VK_F5, Key::F5)
        MAP_KEY(VK_F6, Key::F6)
        MAP_KEY(VK_F7, Key::F7)
        MAP_KEY(VK_F8, Key::F8)
        MAP_KEY(VK_F9, Key::F9)
        MAP_KEY(VK_F10, Key::F10)
        MAP_KEY(VK_F11, Key::F11)
        MAP_KEY(VK_F12, Key::F12)

        // Numlock
        MAP_KEY(VK_NUMLOCK, Key::Numlockclear)
        MAP_KEY(VK_SCROLL, Key::Scrolllock)

        MAP_KEY(VK_OEM_PLUS, Key::Plus)
        MAP_KEY(VK_OEM_COMMA, Key::Comma)
        MAP_KEY(VK_OEM_MINUS, Key::Minus)
        MAP_KEY(VK_OEM_PERIOD, Key::Period)
        MAP_KEY(VK_OEM_2, Key::Slash)
    }

    KeyEvent event(type, keycode, mod_state);
    widget->handle(event);

    if (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
        // MSDN says SYS need to be handled by default.
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}
RECT Win32Window::adjust_rect(int x, int y, int w, int h) {
    RECT rect;
    rect.left = x;
    rect.top  = y;
    rect.right = x + w;
    rect.bottom = y + h;
    driver->AdjustWindowRectExForDpi(&rect, style(), has_menu, exstyle(), driver->GetDpiForWindow(hwnd));
    return rect;
}
int Win32Window::client_to_btk(int v) const { 
    return MulDiv(v, 96, win_dpi);
}
int Win32Window::btk_to_client(int v) const {
    return MulDiv(v, win_dpi, 96);
}
POINTS Win32Window::client_to_btk(POINTS s) const {
    s.x = client_to_btk(s.x);
    s.y = client_to_btk(s.y);
    return s;
}
POINTS Win32Window::btk_to_client(POINTS s) const {
    s.x = btk_to_client(s.x);
    s.y = btk_to_client(s.y);
    return s;
}
DWORD  Win32Window::style() const {
    return GetWindowLongPtrW(hwnd, GWL_STYLE);
}
DWORD  Win32Window::exstyle() const {
    return GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
}

// Win32GLContext
Win32GLContext::Win32GLContext(HWND h) {
    hwnd  = h;
    hdc   = GetDC(h);
}
Win32GLContext::~Win32GLContext() {
    if (hglrc) {
        wglDeleteContext(hglrc);
    }

    // Release library
    FreeLibrary(library);
}
void Win32GLContext::begin() {
    auto prev_rc = wglGetCurrentContext();
    auto prev_dc = wglGetCurrentDC();

    if (prev_rc != hglrc) {
        if (hwnd != nullptr) {
            // hdc = GetDC(hwnd);
        }
        wglMakeCurrent(hdc, hglrc);
    }
    else {
        // Let end()
        prev_rc = nullptr;
    }
}
void Win32GLContext::end() {
    if (hwnd != nullptr) {
        // ReleaseDC(hwnd, hdc);
    }

    if (prev_rc) {
        wglMakeCurrent(prev_dc, prev_rc);
    }
}
void Win32GLContext::swap_buffers() {
    SwapBuffers(hdc);
}
bool Win32GLContext::initialize(const GLFormat &format) {
    PIXELFORMATDESCRIPTOR d;
    Btk_memset(&d, 0, sizeof(d));
    d.nSize = sizeof(d);

    d.nVersion = 1;
    d.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    d.iPixelType = PFD_TYPE_RGBA;
    d.cColorBits = 32;
    d.cDepthBits = format.depth_size;
    d.cStencilBits = format.stencil_size;
    d.iLayerType = PFD_MAIN_PLANE;

    // if (hwnd) {
    //     hdc = GetDC(hwnd);
    // }

    auto pf = ChoosePixelFormat(hdc, &d);
    if (pf == 0) {
        return false;
    }

    if (!SetPixelFormat(hdc, pf, &d)) {
        return false;
    }
    hglrc = wglCreateContext(hdc);
    if (hglrc == nullptr) {
        return false;
    }

    // Get extension function pointers
    wglSetSwapIntervalEXT = (PFNwglSetSwapIntervalEXT) wglGetProcAddress("wglSwapIntervalEXT");

    // Clenup
    // if (hwnd) {
    //     ReleaseDC(hwnd, hdc);
    // }
    return true;
}
bool Win32GLContext::make_current() {
    return wglMakeCurrent(hdc, hglrc);
}
bool Win32GLContext::set_swap_interval(int v) {
    if (wglSetSwapIntervalEXT) {
        return false;
    }
    begin();
    auto ret = wglSetSwapIntervalEXT(v);
    end();
    return ret;
}
Size Win32GLContext::get_drawable_size() {
    // Get drawable size
    RECT rect;
    GetClientRect(hwnd, &rect);
    return Size(rect.right - rect.left, rect.bottom - rect.top);
}
void *Win32GLContext::get_proc_address(const char_t *name) {
    if (wglGetProcAddress) {
        begin();
        auto proc = wglGetProcAddress(name);
        end();
        // Got it 
        if (proc) {
            return reinterpret_cast<void*>(proc);
        }
        // Possible normal OpenGL function
    }
    return reinterpret_cast<void*>(GetProcAddress(library, name));
}

Win32Cursor::Win32Cursor(const PixBuffer &pixbuf, int hot_x, int hot_y) {
    ICONINFO info = {};
    info.fIcon    = FALSE;
    info.xHotspot = hot_x;
    info.yHotspot = hot_y;
    info.hbmColor = buffer_to_hbitmap(pixbuf);
    info.hbmMask  = CreateBitmap(pixbuf.width(), pixbuf.height(), 1, 1, nullptr);

    auto icon = CreateIconIndirect(&info);

    DeleteObject(info.hbmColor);
    DeleteObject(info.hbmMask);

    cursor = (HCURSOR)CopyImage(icon, IMAGE_CURSOR, pixbuf.width(), pixbuf.height(), 0);
    DestroyIcon(icon);

    owned = true;
}
Win32Cursor::Win32Cursor(SystemCursor sys) {
    auto def = IDC_ARROW;
    switch (sys) {
        case SystemCursor::Arrow : def = IDC_ARROW; break;
        case SystemCursor::Crosshair : def = IDC_CROSS; break;
        case SystemCursor::Wait : def = IDC_WAIT; break;
        case SystemCursor::Ibeam : def = IDC_IBEAM; break;
        case SystemCursor::SizeNwse : def = IDC_SIZENWSE; break;
        case SystemCursor::SizeNesw : def = IDC_SIZENESW; break;
        case SystemCursor::SizeWe : def = IDC_SIZEWE; break;
        case SystemCursor::SizeNs : def = IDC_SIZENS; break;
        case SystemCursor::SizeAll : def = IDC_SIZEALL; break;
        case SystemCursor::No : def = IDC_NO; break;
        case SystemCursor::Hand : def = IDC_HAND; break;
    }
    refcount = std::numeric_limits<int>::min();
    owned = false;
    cursor = LoadCursor(nullptr, def);
}
Win32Cursor::~Win32Cursor() {
    if (owned) {
        DestroyCursor(cursor);
    }
}
void Win32Cursor::ref() {
    // Unwork for SystemCursor
    if (refcount == std::numeric_limits<int>::min()) {
        return;
    }

    ++refcount;
}
void Win32Cursor::unref() {
    // Unwork for SystemCursor
    if (refcount == std::numeric_limits<int>::min()){
        return;
    }
    if (--refcount <= 0) {
        delete this;
    }
}
void Win32Cursor::set() {
    SetCursor(cursor);
}

BTK_PRIV_END

BTK_NS_BEGIN

GraphicsDriverInfo Win32DriverInfo = {
    []() -> GraphicsDriver * {
        return new Win32Driver();
    },
    "Win32"
};

BTK_NS_END