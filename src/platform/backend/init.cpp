#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/context.hpp>

BTK_NS_BEGIN 

// Register From Paint backend
extern "C" void __BtkPlatform_D2D_Init();
extern "C" void __BtkPlatform_NVG_Init();
extern "C" void __BtkPlatform_CAIRO_Init();


extern "C" void __BtkPlatform_Init() {
    static bool inited = false;
    if (inited) {
        return;
    }

#if defined(BTK_DRIVER)
    RegisterDriver(BTK_DRIVER);
#endif

#if defined(BTK_DIRECT2D_PAINTER)
    __BtkPlatform_D2D_Init();
#endif

#if defined(BTK_NANOVG_PAINTER)
    __BtkPlatform_NVG_Init();
#endif

    inited = true;
}

BTK_INITCALL(__BtkPlatform_Init);

BTK_NS_END