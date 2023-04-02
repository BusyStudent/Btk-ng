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
         * @brief Send a notifition to desktop
         * 
         * @return true 
         * @return false 
         */
        virtual bool     notify(u8string_view title, u8string_view msg = { }, const PixBuffer &icon = { }) = 0;
        /**
         * @brief Let system open a url in a native way
         * 
         * @param string 
         * @return true 
         * @return false 
         */
        virtual bool     openurl(u8string_view url) = 0;
    protected:
        ~DesktopService() = default;
};

BTKAPI DesktopService *GetDesktopService();

BTK_NS_END
