#pragma once

#include "build.hpp"
#include "common/dlloader.hpp"
#include <dwmapi.h>

BTK_PRIV_BEGIN

LIB_BEGIN(__Win32Dwmapi, "dwmapi.dll")
    LIB_PROC(DwmEnableBlurBehindWindow)
    LIB_PROC(DwmGetColorizationColor)
    LIB_PROC(DwmIsCompositionEnabled)
    LIB_PROC(DwmExtendFrameIntoClientArea)
LIB_END(__Win32Dwmapi)

class Win32Dwmapi : public __Win32Dwmapi {
    public:
        // Experimental to enable alpha blending
        // Borrowed by from GLFW
        BOOL DwmEnableAlphaBlend(HWND hwnd) {
            // Get version
            BOOL composition = FALSE;
            if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition) {
                return FALSE;
            }

            BOOL opaque;
            DWORD color;
            HRESULT hr;

            if ((SUCCEEDED(DwmGetColorizationColor(&color, &opaque)))) {
                HRGN region = ::CreateRectRgn(0, 0, -1, -1);
                DWM_BLURBEHIND bb = {};
                bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
                bb.hRgnBlur = region;
                bb.fEnable = TRUE;
                hr = DwmEnableBlurBehindWindow(hwnd, &bb);
                ::DeleteObject(region);
                return SUCCEEDED(hr);
            }
            return FALSE;
        }
};


BTK_PRIV_END