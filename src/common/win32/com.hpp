#pragma once

#include "build.hpp"
#include <windows.h>
#include <wrl.h>

#undef min
#undef max

BTK_NS_BEGIN2(BTK_NAMESPACE::Win32)

// Guard for Init Com
class ComInitializer {
    public:
        ComInitializer() {
            inited = SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        }
        ~ComInitializer() {
            if (inited) {
                ::CoUninitialize();
            }
        }
    private:
        bool inited;
};

BTK_NS_END2(BTK_NAMESPACE::Win32)