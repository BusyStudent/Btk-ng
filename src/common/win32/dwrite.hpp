#pragma once

#include "build.hpp"
#include <dwrite.h>


#if __has_include(<dwrite_1.h>)
#define BTK_DWRITE_EXTENSION1
#include <dwrite_1.h>
#endif

#if __has_include(<dwrite_2.h>)
#define BTK_DWRITE_EXTENSION2
#include <dwrite_2.h>
#endif

#if __has_include(<dwrite_3.h>)
#define BTK_DWRITE_EXTENSION3
#include <dwrite_3.h>
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "dwrite.lib")
#endif

BTK_NS_BEGIN2(BTK_NAMESPACE::Win32)

class DWrite {
    private:
        inline static IDWriteFactory *factory = nullptr;
    public:
        static void Init() {
            if (factory) {
                factory->AddRef();
                return;
            }
            HRESULT hr;
            hr = DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&factory)
            );
            BTK_ASSERT(SUCCEEDED(hr));
        }
        static void Shutdown() {
            if (!factory) {
                return;
            }
            if (factory->Release() == 0) {
                factory = nullptr;
            }
        }
        static auto GetInstance() -> IDWriteFactory *{
            return factory;
        }
};
class DWriteInitializer {
    public:
        DWriteInitializer() {
            DWrite::Init();
        }
        ~DWriteInitializer() {
            DWrite::Shutdown();
        }
};

BTK_NS_END2(BTK_NAMESPACE::Win32)