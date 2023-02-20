/**
 * @file direct2d.hpp
 * @author BusyStudent
 * @brief Helper class for direct2d
 * @version 0.1
 * @date 2022-12-15
 * 
 * @copyright Copyright (c) 2022 BusyStudent
 * 
 */
#pragma once

#if defined(__GNUC__)
#define WIDL_EXPLICIT_AGGREGATE_RETURNS
#endif

#include "build.hpp"
#include <windows.h>
#undef DrawText
// Avoid posibility of macro conflict
#include <d2d1.h>

// Include extensions
#if __has_include(<d2d1_1.h>)
#define BTK_DIRECT2D_EXTENSION1
#include <d2d1_1.h>
#endif

#if __has_include(<d2d1_2.h>)
#define BTK_DIRECT2D_EXTENSION2
#include <d2d1_2.h>
#endif

#if __has_include(<d2d1_3.h>)
#define BTK_DIRECT2D_EXTENSION3
#include <d2d1_3.h>
#endif

#if defined(_MSC_VER)
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#endif

BTK_NS_BEGIN2(BTK_NAMESPACE::Win32)

class Direct2D {
    private:
        inline static ID2D1Factory *factory = nullptr;
    public:
        static void   Init() {
            if (!factory) {
                D2D1_FACTORY_OPTIONS options{};

                // Enable debug on debug builds only
                // And MinGW often doesn't support debug layers
#if            !defined(NDEBUG) || !defined(_MSC_VER)
                options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

                auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options, &factory);
                BTK_ASSERT(SUCCEEDED(hr));
                return;
            }
            factory->AddRef();
        }
        static void    Shutdown() {
            if (!factory) {
                return;
            }
            if (factory->Release() == 0) {
                factory = nullptr;
            }
        }
        static auto     GetInstance() -> ID2D1Factory * {
            return factory;
        }
};
class Direct2DInitializer {
    public:
        Direct2DInitializer() {
            Direct2D::Init();
        }
        ~Direct2DInitializer() {
            Direct2D::Shutdown();
        }
};

BTK_NS_END2(BTK_NAMESPACE::Win32)