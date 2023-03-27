#include "build.hpp"

BTK_NS_BEGIN

extern "C" void __BtkWidgets_STYLE_Init();
extern "C" void __BtkWidgets_Init() {
    BTK_ONCE([]() {
        __BtkWidgets_STYLE_Init();
    }());
}
BTK_INITCALL(__BtkWidgets_Init);

BTK_NS_END