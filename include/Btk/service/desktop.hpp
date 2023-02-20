#pragma once

#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

BTK_NS_BEGIN

/**
 * @brief Interface for Desktop Window System like Win32 X11
 * 
 */
class DesktopService {
    public:
        /**
         * @brief Reparent a native window to AbstractWindow
         * 
         * @param parent The pointer of AbstractWindow as parent
         * @param native_window The child window embed to it
         * @param placement The position of the child window (in dpi indepent units)
         * @return true 
         * @return false 
         */
        virtual bool     reparent_native_window(window_t parent, void *native_window, const Rect &placement) = 0;
        /**
         * @brief Let system open a url in a native way
         * 
         * @param string 
         * @return true 
         * @return false 
         */
        virtual bool     openurl(u8string_view url) = 0;
};

BTK_NS_END
