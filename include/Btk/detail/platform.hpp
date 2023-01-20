/**
 * @file platform.hpp
 * @author BusyStudent (fyw90mc@gmail.com)
 * @brief The platform abstract class 
 * @version 0.1
 * @date 2023-01-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <Btk/detail/macro.hpp>
#include <Btk/widget.hpp>
#include <Btk/string.hpp>
#include <Btk/rect.hpp>

BTK_NS_BEGIN

class GraphicsDriverInfo {
    public:
        GraphicsDriver *(*create)();
        const char       *name;
};

class GraphicsDriver : public Any {
    public:
        enum Feature {
            Reparent, //< Can reparent window
            OpenGL, //< Supports OpenGL
            Vulkan, //< Supports Vulkan
            Metal, //< Supports Metal
            MultiWindow, //< Supports create more than one window
        };
        enum Query {
            SystemDpi,   //< System dpi (*FPoint)
            NumOfScreen, //< Number of screen (*int)
        };


        // Window management
        virtual window_t window_create(const char_t * title, int width, int height, WindowFlags flags) = 0;

        // Clipboard
        virtual void     clipboard_set(const char_t * text) = 0;
        virtual u8string clipboard_get() = 0;

        // Cursor
        virtual cursor_t cursor_create(const PixBuffer &pixbuf, int hot_x, int hot_y) = 0;

        // Dynamic factory
        virtual any_t    instance_create(const char_t *what, ...) = 0;

        // Query
        virtual bool     query_value(int query, ...) = 0;
};

// Provide more details of desktop
class DesktopDriver : public GraphicsDriver {
    public:
        // TODO : 
        virtual pointer_t menu_create() = 0;

        // Extend of clipboard
        virtual void      clipboard_set_image(const PixBuffer &) = 0;
        virtual PixBuffer clipboard_get_image() = 0;
};

class AbstractWindow : public Any {
    public:
        enum ShowFlag : int {
            Hide,
            Show,
            Restore,
            Maximize,
            Minimize,
        };
        enum Value : int {
            NativeHandle,
            Hwnd,        //< Win32 Window Handle (*HWND)
            Hdc,         //< Win32 Driver Context (*HDC)
            XConnection, //< Xcb  Connection  (*xcb_connection_t*)
            XDisplay,    //< Xlib Display     (*Display)
            XWindow,     //< Window           (*Window)
            Dpi,         //< Dpi              (*FPoint)
            MousePosition, //< MousePosition  (*Point)
            MaximumSize, //< args (*Size)
            MinimumSize, //< args (*Size)
        };

        virtual Size       size() const = 0;
        virtual Point      position() const = 0;
        virtual void       raise() = 0;
        // virtual void       grab() = 0;
        virtual void       close() = 0; //< Send close event to window
        virtual void       repaint() = 0;
        virtual void       show(int show_flag) = 0;
        virtual void       move  (int x, int y) = 0;
        virtual void       resize(int width, int height) = 0;
        virtual void       set_title(const char_t * title) = 0;
        virtual void       set_icon(const PixBuffer &buffer) = 0;
        virtual void       set_textinput_rect(const Rect &rect) = 0;
        virtual void       capture_mouse(bool capture) = 0;
        virtual void       start_textinput(bool start) = 0;

        // Flags control
        virtual bool       set_flags(WindowFlags flags) = 0;
        virtual bool       set_value(int conf, ...) = 0;
        virtual bool       query_value(int query, ...) = 0;

        virtual widget_t   bind_widget(widget_t widget) = 0;
        virtual any_t      gc_create(const char_t *type) = 0;
        // [[deprecated("Use Painter::FromWindow()")]]
        // virtual Painter    painter_create() = 0;
};

class AbstractCursor : public Any {
    public:
        // Refcounting manage
        virtual void       ref()   = 0;
        virtual void       unref() = 0;

        virtual void       set() = 0;
};
class AbstractScreen : public Any {
    public:
        virtual Size       pixel_size() = 0;
        virtual FPoint     dpi()        = 0;
};

using      EventDtor = void (*)(Event *);
enum class EventWalk {
    Continue = 0,
    Stop     = 1,
    Drop     = 1 << 1,
};

BTK_FLAGS_OPERATOR(EventWalk, int);
/**
 * @brief Interface to access EventQueue
 * 
 */
class EventDispatcher {
    public:
        // Timer
        virtual timerid_t timer_add(Object *object, timertype_t type, uint32_t ms) = 0;
        virtual bool      timer_del(Object *object, timerid_t id) = 0;

        virtual bool      send(Event *event, EventDtor dtor = nullptr) = 0;
        virtual pointer_t alloc(size_t n) = 0;

        virtual void      interrupt() = 0;
        virtual int       run() = 0;

        template <typename T>
        inline  bool      send(T &&event) {
            // Construct event
            using Ty = std::decay_t<T>;
            auto addr = alloc(sizeof(Ty));
            new (addr) Ty(std::forward<T>(event));

            if constexpr (std::is_trivially_destructible_v<Ty>) {
                return send(static_cast<Event*>(addr), nullptr);
            }
            else {
                return send(static_cast<Event*>(addr), [](Event *ev) {
                    static_cast<Ty*>(ev)->~Ty();
                });
            }
        }

        BTKAPI bool      dispatch(Event *);
};

extern GraphicsDriverInfo Win32DriverInfo;
extern GraphicsDriverInfo SDLDriverInfo;
extern GraphicsDriverInfo XcbDriverInfo;

BTKAPI auto RegisterDriver(GraphicsDriverInfo &) -> void;
BTKAPI auto CreateDriver()                       -> GraphicsDriver *;

BTK_NS_END