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
        enum Service {
            Desktop, //< Support desktop service  
        };
        enum Query {
            SystemDpi,   //< System dpi (*FPoint)
            NumOfScreen, //< Number of screen (*int)
            KeyFocusWindow,   //< The widget has “keyboard” focus (*AbstractWindow*)
            MouseFocusWindow, 	//< The widget has “mouse”  focus (*AbstractWindow*)
        };


        // Window management
        virtual window_t window_create(u8string_view title, int width, int height, WindowFlags flags) = 0;

        // Clipboard
        virtual void     clipboard_set(u8string_view text) = 0;
        virtual u8string clipboard_get() = 0;

        // Cursor
        /**
         * @brief Create cursor from pixel buffer (you will take it's ownship, call unref to release it)
         * 
         * @param pixbuf 
         * @param hot_x 
         * @param hot_y 
         * @return cursor_t 
         */
        virtual cursor_t cursor_create(const PixBuffer &pixbuf, int hot_x, int hot_y) = 0;
        virtual cursor_t cursor_create(SystemCursor syscursor) = 0;

        // Dynamic factory
        virtual any_t    instance_create(const char_t *what, ...) = 0;

        // Query
        virtual bool     query_value(int query, ...) = 0;

        // Get service interface
        virtual pointer_t service_of(int what) = 0;

        // Template helper
        template <typename T>
        inline T *service_of();
};
/**
 * @brief Window abstraction
 * 
 */
class AbstractWindow : public Any {
    public:
        enum ShowFlag : int {
            Hide,
            Show,
            Restore,
            Maximize,
            Minimize,
        };
        enum MapPoint : int {
            ToScreen, //< ClientToScreen
            ToClient, //< ScreenToClient
            ToPixel,  //< Map dpi indepent units to pixel unit
            ToDIPS,   //< Map pixel unit to dpi indepent units
        };
        enum Value : int {
            NativeHandle, //< Native Window Handle (*HWND in Win32, *Window in X11)
            Hwnd,        //< Win32 Window Handle (*HWND)
            Hdc,         //< Win32 Driver Context (*HDC)
            SDLWindow,   //< SDL Window Handle (*SDL_Window*)
            XConnection, //< Xcb  Connection  (*xcb_connection_t*)
            XDisplay,    //< Xlib Display     (*Display)
            XWindow,     //< Window           (*Window)
            Dpi,         //< Dpi              (*FPoint)
            MousePosition, //< MousePosition  (*Point)
            MaximumSize, //< args (*Size)
            MinimumSize, //< args (*Size)
            Opacity,     //< args (*float)
        };

        /**
         * @brief Get the size of the window, in dpi indepent units
         * 
         * @return Size 
         */
        virtual Size       size() const = 0;
        /**
         * @brief Get the position of the window, in dpi indepent units
         * 
         * @return Point 
         */
        virtual Point      position() const = 0;
        /**
         * @brief Raise the window to the top of it
         * 
         */
        virtual void       raise() = 0;
        // virtual void       grab() = 0;
        /**
         * @brief Send a close event to the window
         * 
         */
        virtual void       close() = 0;
        /**
         * @brief Send a repaint event to the window
         * 
         */
        virtual void       repaint() = 0;
        /**
         * @brief Show the window by flag
         * 
         * @param show_flag 
         */
        virtual void       show(int show_flag) = 0;
        /**
         * @brief Move window to x, y (in dpi indepent units)
         * 
         * @param x 
         * @param y 
         */
        virtual void       move  (int x, int y) = 0;
        /**
         * @brief Resize window to x, y (in dpi indepent units)
         * 
         * @param width 
         * @param height 
         */
        virtual void       resize(int width, int height) = 0;
        /**
         * @brief Set the title object
         * 
         * @param title 
         */
        virtual void       set_title(u8string_view title) = 0;
        /**
         * @brief Set the icon object
         * 
         * @param buffer 
         */
        virtual void       set_icon(const PixBuffer &buffer) = 0;
        /**
         * @brief Set the textinput rect object (ime rect hit)
         * 
         * @param rect 
         */
        virtual void       set_textinput_rect(const Rect &rect) = 0;
        /**
         * @brief capture the mouse
         * 
         * @param capture 
         */
        virtual void       capture_mouse(bool capture) = 0;
        /**
         * @brief Set the status of the ime
         * 
         * @param start 
         */
        virtual void       start_textinput(bool start) = 0;

        /**
         * @brief Set the flags object
         * 
         * @param flags The window flags
         * @return true 
         * @return false 
         */
        virtual bool       set_flags(WindowFlags flags) = 0;
        /**
         * @brief Set the value object
         * 
         * @param conf 
         * @param ... 
         * @return true Ok
         * @return false Failed or unsupported
         */
        virtual bool       set_value(int conf, ...) = 0;
        /**
         * @brief Query the value
         * 
         * @param query 
         * @param ... 
         * @return true Ok
         * @return false Failed or unsupported
         */
        virtual bool       query_value(int query, ...) = 0;

        /**
         * @brief Map a point to the target coord system
         * 
         * @param p The input point
         * @param type The type of the convertion
         * @return Point The result point
         */
        virtual Point      map_point(Point p, int type) = 0;
        /**
         * @brief Bind a object to receive events from Window System
         * 
         * @param object The pointer of object
         * @return widget_t The previous object pointer
         */
        virtual widget_t   bind_widget(widget_t object) = 0;
        /**
         * @brief Create a Graphics context from type
         * 
         * @param type 
         * @return any_t 
         */
        virtual any_t      gc_create(const char_t *type) = 0;
};

class AbstractCursor : public DynRefable {
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

// Impl helper
template <>
inline DesktopService *GraphicsDriver::service_of<DesktopService>() {
    return static_cast<DesktopService*>(service_of(Desktop));
}

BTK_NS_END