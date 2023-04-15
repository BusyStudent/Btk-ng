#pragma once

#include "build.hpp"
#include "common/dlloader.hpp"
#include <dwmapi.h>

BTK_PRIV_BEGIN

LIB_BEGIN(__Win32Dwmapi, "dwmapi.dll")
    LIB_PROC(DwmEnableBlurBehindWindow)
    LIB_PROC(DwmGetColorizationColor)
    LIB_PROC(DwmIsCompositionEnabled)
    LIB_PROC(DwmSetWindowAttribute)
    LIB_PROC(DwmExtendFrameIntoClientArea)
LIB_END(__Win32Dwmapi)

class Win32Dwmapi : public __Win32Dwmapi {
    public:
        // Experimental to enable alpha blending
        // Borrowed by from GLFW
        BOOL DwmEnableAlphaBlend(HWND hwnd) const {
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
        BOOL DwmSetDropShadow(HWND hwnd, BOOL enable) const {
            BOOL composition = FALSE;
            if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition) {
                return FALSE;
            }

            DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
            HRESULT hr;

            hr = DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
            if (FAILED(hr)) {
                return false;
            }

            MARGINS m{0};
            if (enable) {
                m = MARGINS {1, 1, 1, 1};
                // m = MARGINS {-1, -1, -1, -1};
            }
            hr = DwmExtendFrameIntoClientArea(hwnd, &m);

            return SUCCEEDED(hr);
        }
};


BTK_PRIV_END