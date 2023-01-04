#include "build.hpp"

#include <Btk/context.hpp>

BTK_NS_BEGIN 

// It is so ugly , need a better solution

GraphicsDriverInfo PlatformDriverInfo = {
    [] () {
        return BTK_DRIVER.create();
    },
    BTK_STRINGIFY(BTK_DRIVER)
};

BTK_NS_END