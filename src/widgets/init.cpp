#include "build.hpp"

BTK_NS_BEGIN

extern "C" void __BtkWidgets_STYLE_Init();
extern "C" void __BtkWidgets_Init() {
    BTK_ONCE([]() {
        __BtkWidgets_STYLE_Init();
    }());
}

#if defined(BTK_DLL)
    BTK_INITCALL(__BtkWidgets_Init);
#endif

BTK_NS_END