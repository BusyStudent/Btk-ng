#include "build.hpp"

#include <Btk/context.hpp>
#include <unordered_map>
#include <Windows.h>
#include <wingdi.h>
#include <csignal>
#include <memory>
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
// Import backtrace on MSVC (for debugging)
#if !defined(NDEBUG)
#pragma comment(lib, "advapi32.lib")
#include "libs/StackWalker.cpp"
#endif
#endif

#undef min
#undef max

#define WM_CALL    (WM_APP + 0x100)
#define WM_REPAINT (WM_APP + 0x101)

// OpenGL support
#define GL_LIB(x)    HMODULE library = LoadLibraryA(x)
#define GL_TYPE(x)   decltype(::x)
#define GL_PROC(x)   GL_TYPE(x) *x = (GL_TYPE(x)*) GetProcAddress(library, #x) 

// Timer support
#define TIMER_PRECISE (UINTPTR_MAX / 2)
#define TIMER_COARSE  (              0)

#define TIMER_IS_PRECISE(x) ((x) >= TIMER_PRECISE)
#define TIMER_IS_COARSE(x)  ((x) <  TIMER_PRECISE)

// Debug show key translation strings
#if !defined(NDEBUG)
#define SHOW_KEY(vk, k) printf(vk); printf(" => "); printf(k); printf("\n");
#define MAP_KEY(vk, k) case vk : { SHOW_KEY(#vk, #k) ; keycode = k; break;}
#define WIN_INFO(...) printf(__VA_ARGS__); printf("\n");
#else
#define MAP_KEY(vk, k) case vk : { keycode = k; break; }
#define WIN_INFO(...)
#endif

using Microsoft::WRL::ComPtr;
using Win32Callback = void (*)(void *param);

// Make sure the MSG can be casted to a Win32Callback
static_assert(sizeof(LPARAM) >= sizeof(void *));
static_assert(sizeof(WPARAM) >= sizeof(void *));
// Make sure is utf16
static_assert(sizeof(wchar_t) == sizeof(uint16_t));

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

class Win32Driver ;
class Win32Window ;

class Win32Window : public AbstractWindow {
    public:
        Win32Window(HWND h, Win32Driver *d);
        ~Win32Window();

        widget_t bind_widget(widget_t w) override;

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
        // IME support
        void       start_textinput(bool v) override;
        void       set_textinput_rect(const Rect &r) override;


        Painter    painter_create() override;

        LRESULT    proc_keyboard(UINT msg, WPARAM wp, LPARAM lp);

        RECT       adjust_rect(int x, int y, int w, int h);

        HWND         hwnd   = {};
        Win32Driver *driver = {};
        widget_t     widget = {};
        Win32Window *parent = {}; //< For embedding

        bool         mouse_enter = false;
        bool         has_menu  = false;
        Modifier     mod_state = Modifier::None; //< Current keyboard modifiers state
        DWORD        win_style = 0; //< Window style

        HMENU        menu = {}; //< Menu handle
        
        Size         maximum_size = {-1, -1}; //< Maximum window size
        Size         minimum_size = {-1, -1}; //< Minimum window size

        SHORT        mouse_last_x = -1; //< Last mouse position
        SHORT        mouse_last_y = -1; //< Last mouse position

        DWORD64      last_repaint = 0; //< Last repaint time
        DWORD        repaint_limit = 60; //< Repaint limit in fps
        bool         repaint_in_progress = false; //< Repaint in progress
        bool         textinput_enabled = false; //< Text input enabled

        HIMC         imcontext = {}; //< Default IME context
};

class Win32Driver : public GraphicsDriver {
    public:
        Win32Driver();
        ~Win32Driver();


        window_t window_create(const char_t *title, int width, int height, WindowFlags flags) override;
        void     window_add(Win32Window *w);
        void     window_del(Win32Window *w);

        void     clipboard_set(const char_t *text) override;
        u8string clipboard_get() override;

        timerid_t timer_add(Object *object, timertype_t type, uint32_t ms) override;
        bool      timer_del(Object *object, timerid_t id) override;

        LRESULT   wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        LRESULT   helper_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


        void      pump_events(UIContext *ctxt) override;
        

        std::unordered_map<HWND, Win32Window*> windows;

        // TODO : a better way to do this?
        std::unordered_map<timerid_t, Object*> timers;

        UIContext *context = {};

        // Platform data
        DWORD      working_thread_id = GetCurrentThreadId();
        HWND       helper_hwnd = {}; //< Helper window for timer
};

class Win32GLContext : public GLContext {
    public:
        void begin() override;
        void end() override;
        void swap_buffers() override;

        bool initialize(const GLFormat &format) override;
    private:
        HDC          hdc    = {};
        HGLRC        hglrc  = {};
        Win32Driver *driver = {};
        Win32Window *window = {};

        // OpenGL functions pointers
        GL_LIB      ("opengl32.dll");
        GL_PROC     (wglMakeCurrent);
        GL_PROC     (wglCreateContext);
        GL_PROC     (wglDeleteContext);
        GL_PROC     (wglGetProcAddress);
        GL_PROC     (wglGetCurrentContext);
};

// For timer stored at object
class Win32Timer : public Any {
    public:
        HANDLE handle;
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
            HRESULT hr = CoCreateInstance(
                IID_ISynchronize,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_ISynchronize,
                (void**)&sync
            );

            assert(SUCCEEDED(hr));
        }


        RetT result() const {
            sync->Wait(COWAIT_DEFAULT, INFINITE);
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
            call->sync->Signal();
        }
    private:
        std::conditional_t<
            std::is_same_v<RetT, void>,
            int,
            RetT
        > ret;
        ComPtr<ISynchronize> sync;
        Callable callable;
};

// Global instance of the driver.
static Win32Driver *win32_driver = nullptr;

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
    PostMessage(
        win32_driver->helper_hwnd,
        WM_CALL,
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


Win32Driver::Win32Driver() {
    // Initialize the window class.
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND w, UINT m, WPARAM wp, LPARAM lp) {
        return win32_driver->wnd_proc(w, m, wp, lp);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"BtkWindow";
    wc.hCursor = LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hIcon = LoadIcon(nullptr,IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr,IDI_APPLICATION);
    RegisterClassExW(&wc);


    // Initialize helper window class
    wc.lpszClassName = L"BtkHelper";
    wc.lpfnWndProc = [](HWND w, UINT m, WPARAM wp, LPARAM lp) {
        return win32_driver->helper_wnd_proc(w, m, wp, lp);
    };

    // Reg it
    RegisterClassExW(&wc);

    // Create Helper window
    helper_hwnd = CreateWindowExW(
        0,
        L"BtkHelper",
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

    assert(helper_hwnd);

    // Initialize the IME

    // Set the instance
    win32_driver = this;

#if !defined(NDEBUG)
    // Initialize the debug 
    #pragma comment(lib, "dbghelp.lib")
    SetUnhandledExceptionFilter([](_EXCEPTION_POINTERS *info) -> LONG {
        // Dump callstack
        static bool recursive = false;
        if (recursive) {
            ExitProcess(EXIT_FAILURE);
            return EXCEPTION_CONTINUE_SEARCH;
        }
        recursive = true;

#if     defined(_MSC_VER)
        class Walk : public StackWalker {
            public:
                Walk(std::string &out) : output(out) {}
                void OnCallstackEntry(CallstackEntryType,CallstackEntry &addr) override {
                    char buffer[1024] = {};
                    // Format console output
                    if (addr.lineNumber == 0) {
                        // No line number
                        sprintf(buffer, "#%d %p at %s [%s]\n", n, addr.offset, addr.name, addr.moduleName);
                    }
                    else {
                        sprintf(buffer, "#%d %p at %s [%s:%d]\n", n, addr.offset, addr.name, addr.lineFileName, addr.lineNumber);
                    }
                    fputs(buffer, stderr);
                    // Format message box output
                    sprintf(buffer, "#%d %p at %s\n", n, addr.offset, addr.name);
                    output.append(buffer);
                    n += 1;
                }
            private:
                std::string &output;
                int       n = 0;
        };
        std::string output = "-- Callstack summary --\n";
        // Set Console color to red
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED);
        fputs("--- Begin Callstack ---\n", stderr);
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        Walk(output).ShowCallstack();
        output.append("-- Detail callstack is on the console. ---\n");
        
        // Set Console color to white
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED);
        fputs("--- End Callstack ---\n", stderr);
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        // Show MessageBox
        MessageBoxA(nullptr, output.c_str(), "Unhandled Exception", MB_ICONERROR);
#else
        MessageBoxA(nullptr, "Exception" ,  "Unhandled Exception", MB_ICONERROR);
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    });
    // Hook SIGABRT to dump callstack.
    signal(SIGABRT, [](int) {
        // Dump callstack
        // Raise SEH exception to dump callstack.
        printf("SIGABRT\n");
        RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
    });
#endif

}
Win32Driver::~Win32Driver() {
    DestroyWindow(helper_hwnd);
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
        case WM_QUIT : {
            context->send_event(QuitEvent(wparam));
            break;
        }
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
                    BTK_LOG("Quitting\n");
                    PostQuitMessage(0);
                    
                    Event e(Event::Quit);
                    context->send_event(e);
                }
            }
            break;
        }
        // Repaint from BtkWindow
        case WM_REPAINT : {
            win->repaint_in_progress = false;
            [[fallthrough]];
        }
        case WM_PAINT : {
            WidgetEvent event(Event::Paint);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            widget->handle(event);
            // Draw children
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_SIZE : {
            ResizeEvent event;
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            auto [w, h] = win->size();

            if (w == 0 || h == 0) {
                // Probably mini, ignore it
                BTK_LOG("Resize to %d, %d\n", w, h);
                break;
            }

            event.set_new_size(
                // LOWORD(lparam), HIWORD(lparam)
                w, h
            );

            widget->handle(event);
            break;
        }
        // case WM_GETMINMAXINFO : {
        //     MINMAXINFO *mmi = (MINMAXINFO*)lparam;
        //     if (win->maximum_size.is_valid()) {

        //     }
        //     break;
        // }
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
                BTK_LOG("Mouse enter\n");
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

            auto point = MAKEPOINTS(lparam);
            MotionEvent event(point.x, point.y);
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());

            if (win->mouse_last_x != -1 && win->mouse_last_y != -1) {
                event.set_rel(
                    point.x - win->mouse_last_x,
                    point.y - win->mouse_last_y
                );
            }

            // Update previous position
            win->mouse_last_x = point.x;
            win->mouse_last_y = point.y;

            widget->handle(event);
            break;
        }
        case WM_MOUSELEAVE : {
            BTK_LOG("Mouse leave\n");
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
        case WM_LBUTTONDOWN : {
            [[fallthrough]];
        }
        case WM_RBUTTONDOWN : {
            auto point = MAKEPOINTS(lparam);
            MouseEvent event(
                Event::MousePress,
                point.x, point.y,
                wparam
            );
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }
        case WM_LBUTTONUP : {
            [[fallthrough]];
        }
        case WM_RBUTTONUP : {
            auto pos = MAKEPOINTS(lparam);
            MouseEvent event(Event::MouseRelease, pos.x, pos.y, wparam);
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

            // Get drop string
            UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            BTK_LOG("Drop count: %d\n", count);

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
            BTK_LOG("WM_IME_CHAR char: %d\n", wparam);
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
            //     BTK_LOG("Text input disabled, Drop it\n");
            //     break;
            // }
            // Check the range of the char
            if (!win->textinput_enabled) {
                BTK_LOG("TextInput disabled, Drop it\n");
                break;
            }
            if (wparam < 0x20 || wparam > 0x7E) {
                BTK_LOG("Drop char: %d\n", wparam);
                break;
            }
            else {
                BTK_LOG("WM_CHAR : %d\n", wparam);
            }

            char16_t c = wparam;
            TextInputEvent event(u8string::from(&c, 1));
            event.set_widget(win->widget);
            event.set_timestamp(GetTicks());
            widget->handle(event);
            break;
        }

        default : {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }
    return 0;
}
LRESULT Win32Driver::helper_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_TIMER : {
            // Check if is timer event , and it is on the list of timers.
            if (timers.find(wparam) != timers.end()) {

                auto id     = wparam;
                auto object = timers[id];

                TimerEvent event(object, id);
                event.set_timestamp(GetTicks());

                object->handle(event);
                return 0;
            }
            else {
                //??? 
                BTK_LOG("Unregitered WM_TIMER id => %d\n", int(wparam));
            }
            return 0;
        }
        case WM_CALL : {
            auto callback = reinterpret_cast<Win32Callback>(wparam);
            auto param    = reinterpret_cast<void *>(lparam);
            callback(param);
            return 0;
        }
        default : {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }
}
window_t Win32Driver::window_create(const char_t *title, int width, int height, WindowFlags flags) {
    if (working_thread_id != GetCurrentThreadId()) {
        return gui_thread_call([&, this](){
            return window_create(title, width, height, flags);
        });
    }


    // Alloc tmp space for the title.
    wchar_t *wtitle = nullptr;
    
    if (title) {
        size_t len = strlen(title);
        wtitle = (wchar_t *)_malloca(sizeof(wchar_t) * (len + 1));
        utf8_to_utf16(title, len, wtitle, len + 1);
    }

    // Check args if
    DWORD style = WS_OVERLAPPEDWINDOW;
    UINT  dpi   = GetDpiForSystem();

    // Parse flags

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.bottom = height;
    rect.right = width;
    if (!AdjustWindowRectExForDpi(&rect, style, FALSE, style, dpi)) {
        BTK_LOG("Bad adjust");
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    BTK_LOG("Creating window %d %d\n", width, height);


    HWND hwnd = CreateWindowExW(
        0,
        L"BtkWindow",
        wtitle,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    _freea(wtitle);

    if (!hwnd) {
        return nullptr;
    }

    // Configure window
    DragAcceptFiles(hwnd, TRUE);

    auto win = new Win32Window(hwnd, this);
    win->win_style = style;
    return win;
}
void  Win32Driver::window_add(Win32Window *win) {
    windows[win->hwnd] = win;
}
void  Win32Driver::window_del(Win32Window *win) {
    windows.erase(win->hwnd);
}
void  Win32Driver::pump_events(UIContext *ctxt) {
    context = ctxt;
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
void  Win32Driver::clipboard_set(const char_t *text) {
    if (!OpenClipboard(helper_hwnd)) {
        return;
    }
    if (!text) {
        return;
    }
    size_t len = strlen(text);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (len + 1));
    if (!hmem) {
        return;
    }
    wchar_t *mem = (wchar_t *)GlobalLock(hmem);
    utf8_to_utf16(text, len, mem, len + 1);
    GlobalUnlock(hmem);
    SetClipboardData(CF_UNICODETEXT, hmem);

    CloseClipboard();
}
u8string Win32Driver::clipboard_get() {
    u8string text;
    if (OpenClipboard(helper_hwnd)) {
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
// TODO: Timers
timerid_t Win32Driver::timer_add(Object *object, timertype_t t, uint32_t ms) {
    timerid_t id;
    timerid_t offset = 0;
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
            BTK_LOG("SetTimer failed\n");
            return 0;
        }
    }
    else {
        // Use winapi
        auto cb = [](void *id, BOOLEAN) -> void {
            PostMessageW(
                win32_driver->helper_hwnd,
                WM_TIMER,
                reinterpret_cast<WPARAM>(id),
                0
            );
        };
        HANDLE t;
        if (!CreateTimerQueueTimer(&t, nullptr, cb, reinterpret_cast<void*>(id), ms, ms, WT_EXECUTEDEFAULT)) {
            // Unsupported ?
            BTK_LOG("CreateTimerQueueTimer failed");
            // Back to coarse timer
            return timer_add(object, timertype_t::Coarse, ms);
        }
        // Store to object
        auto name = u8string::format("_wt_%p", id);
        auto wt   = new Win32Timer;
        wt->handle = t;
        object->set_userdata(name.c_str(), wt);
    }

    // Register to timer map
    timers[id] = object;
    return id;
}
bool Win32Driver::timer_del(Object *object, timerid_t id) {
    timers.erase(id);
    if (TIMER_IS_COARSE(id)) {
        // Create from SetTimer
        return KillTimer(helper_hwnd, id);
    }
    // Create from CreateTimerQueueTimer
    BTK_LOG("del timer %p\n", id);
    auto name = u8string::format("_wt_%p", id);
    auto wt   = (Win32Timer *)object->userdata(name.c_str());
    // Make sure it is our timer
    assert(wt);
    if (!wt) {
        return false;
    }
    // Delete timer
    BOOL v = DeleteTimerQueueTimer(nullptr, wt->handle, nullptr);
    // Remove from object
    delete wt;
    object->set_userdata(name.c_str(), nullptr);
    return v;
}

Win32Window::Win32Window(HWND h, Win32Driver *d) {
    hwnd = h;
    driver = d;

    driver->window_add(this);

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
    return Size(r.right - r.left, r.bottom - r.top);
}
Point Win32Window::position() const {
    RECT r;
    GetClientRect(hwnd, &r);
    return Point(r.left, r.top);
}
void Win32Window::raise() {
    SetForegroundWindow(hwnd);
}
void Win32Window::close() {
    PostMessageW(hwnd, WM_CLOSE, 0, 0);
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
void Win32Window::resize(int w, int h) {
    auto [x, y] = position();
    auto rect   = adjust_rect(x, y, w, h);

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
void Win32Window::show(bool v) {
    if (v) {
        // Check if the window is on device.
        if (driver->windows.find(hwnd) == driver->windows.end()) {
            driver->window_add(this);
        }
    }

    ShowWindow(hwnd, v ? SW_SHOW : SW_HIDE);
}
void Win32Window::set_title(const char_t * title) {
    wchar_t *wtitle = nullptr;
    if (title) {
        size_t len = strlen(title);
        wtitle = (wchar_t *)_malloca(sizeof(wchar_t) * (len + 1));
        utf8_to_utf16(title, len, wtitle, len + 1);
    }
    SetWindowTextW(hwnd, wtitle);
    _freea(wtitle);
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
            BTK_LOG("CreateIconIndirect failed\n");
            BTK_LOG("GetLastError: %d\n", GetLastError());
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
void     Win32Window::set_textinput_rect(const Rect &rect) {
    auto imc = ImmGetContext(hwnd);
    if (imc) {
        COMPOSITIONFORM cf = {};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = rect.x;
        cf.ptCurrentPos.y = rect.y + rect.h;
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

Painter Win32Window::painter_create() {
    return Painter::FromHwnd(hwnd);
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
        BTK_LOG("mod_state: %x\n", mod_state);
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
                BTK_LOG ("VK_RSHIFT\n");
                proc_mod(Modifier::Rshift);
            }
            else {
                BTK_LOG ("VK_LSHIFT\n");
                proc_mod(Modifier::Lshift);
            }
            break;
        }
        case VK_CONTROL : {
            if (is_extended_key) {
                BTK_LOG ("VK_RCONTROL\n");
                proc_mod(Modifier::Rctrl);
            }
            else {
                BTK_LOG ("VK_LCONTROL\n");
                proc_mod(Modifier::Lctrl);
            }
            break;
        }
        case VK_MENU : {
            if (is_extended_key) {
                BTK_LOG ("VK_RMENU\n");
                proc_mod(Modifier::Ralt);
            }
            else {
                BTK_LOG ("VK_LMENU\n");
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
    AdjustWindowRectExForDpi(&rect, win_style, has_menu, 0, GetDpiForWindow(hwnd));
    return rect;
}


BTK_NS_END2()

BTK_NS_BEGIN

GraphicsDriverInfo Win32DriverInfo = {
    []() -> GraphicsDriver * {
        return new Win32Driver();
    },
    "Win32"
};

BTK_NS_END