/**
 * @file dwrite_helper.hpp
 * @author BusyStudent
 * @brief Impl for dwrite helper & dwrite Textlayout Font
 * @version 0.1
 * @date 2022-12-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "build.hpp"
#include "common/win32/wincodec.hpp"
#include "common/win32/direct2d.hpp"
#include <Btk/pixels.hpp>
#include <wincodec.h>
#include <dwrite.h>
#include <wrl.h>

BTK_PRIV_BEGIN

using Microsoft::WRL::ComPtr;

// class DWriteRaster : public IDWriteTextRenderer {
//     public:
//         DWriteRaster(IDWriteFactory *factory, int width, int height);
//         ~DWriteRaster();

//         bool draw(IDWriteTextLayout *layout, float x, float y);

//         // Overrides
//         HRESULT DrawGlyphRun(
//             void* clientDrawingContext,
//             FLOAT baselineOriginX,
//             FLOAT baselineOriginY,
//             DWRITE_MEASURING_MODE measuringMode,
//             DWRITE_GLYPH_RUN const* glyphRun,
//             DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
//             IUnknown* clientDrawingEffect
//         ) noexcept override;
//     private:
//         ComPtr<IDWriteBitmapRenderTarget> target;
//         ComPtr<IDWriteRenderingParams>    params;
//         Color                             color = Color::Black;
// };

// inline DWriteRaster::DWriteRaster(IDWriteFactory *factory, int width, int height) {
//     ComPtr<IDWriteGdiInterop> interop;
//     HRESULT hr = factory->GetGdiInterop(&interop);
//     if (FAILED(hr)) {
//         // Should not failed
//     }
//     hr = interop->CreateBitmapRenderTarget(
//         CreateCompatibleDC(nullptr),
//         width,
//         height,
//         &target
//     );
// }

// inline bool DWriteRaster::draw(IDWriteTextLayout *layout, float x, float y) {
//     HRESULT hr = layout->Draw(
//         this,
//         this,
//         x, y
//     );
//     return SUCCEEDED(hr);
// }

// inline HRESULT DWriteRaster::DrawGlyphRun(
//     void* clientDrawingContext,
//     FLOAT baselineOriginX,
//     FLOAT baselineOriginY,
//     DWRITE_MEASURING_MODE measuringMode,
//     DWRITE_GLYPH_RUN const* glyphRun,
//     DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
//     IUnknown* clientDrawingEffect
// ) noexcept
// {
//     // Forward to GDI 
//     RECT dirty = {0};

//     return target->DrawGlyphRun(
//         baselineOriginX,
//         baselineOriginY,
//         measuringMode,
//         glyphRun,
//         params.Get(),
//         RGB(color.r, color.g, color.b),
//         &dirty
//     );
// }

/**
 * @brief Rasterize DirectWrite TextLayout or String
 * 
 */
class DWriteRaster : private Win32::Direct2DInitializer, Win32::WincodecInitializer {
    public:
        DWriteRaster(PixFormat fmt, int width, int height);
        DWriteRaster(int width, int height);
        DWriteRaster(const DWriteRaster &) = delete;
        ~DWriteRaster();

        bool rasterize(IDWriteTextLayout *layout);
    private:
        bool create();

        ComPtr<ID2D1RenderTarget>    target = {};
        ComPtr<ID2D1SolidColorBrush> brush  = {};
        ComPtr<IWICBitmap>           bitmap = {};
        PixFormat                    fmt    = {};
        int                          width  = 0;
        int                          height = 0;
        bool                         dirty  = true;
        float                        xdpi   = 96;
        float                        ydpi   = 96;
};
inline DWriteRaster::DWriteRaster(PixFormat fmt, int width, int height) : fmt(fmt), width(width), height(height) {

}
inline DWriteRaster::DWriteRaster(int width, int height) : DWriteRaster(PixFormat::Gray8, width, height) {

}
inline DWriteRaster::~DWriteRaster() {

}
inline bool DWriteRaster::create() {
    auto d2d = Win32::Direct2D::GetInstance();
    auto wic = Win32::Wincodec::GetInstance();
    if (fmt != PixFormat::RGBA32 || fmt != PixFormat::Gray8 || width == 0 || height == 0) {
        return false;
    }
    if (!d2d || !wic) {
        return false;
    }

    // Create bitmap
    HRESULT hr;
    hr = wic->CreateBitmap(
        MulDiv(width,  96, xdpi),
        MulDiv(height, 96, ydpi),
        fmt == PixFormat::RGBA32 ? GUID_WICPixelFormat32bppPRGBA : GUID_WICPixelFormat8bppGray,
        WICBitmapCacheOnDemand,
        bitmap.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    // Create render target
    hr = d2d->CreateWicBitmapRenderTarget(
        bitmap.Get(),
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                fmt == PixFormat::RGBA32 ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_A8_UNORM,
                fmt == PixFormat::RGBA32 ? D2D1_ALPHA_MODE_UNKNOWN    : D2D1_ALPHA_MODE_STRAIGHT
            ),
            xdpi, ydpi
        ),
        target.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    // Create brush
    hr = target->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        brush.ReleaseAndGetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    dirty = false;

    return true;
}
inline bool DWriteRaster::rasterize(IDWriteTextLayout *layout) {
    // Measure the size of the text
    DWRITE_TEXT_METRICS metrics;
    HRESULT hr;

    hr = layout->GetMetrics(&metrics);
    if (FAILED(hr)) {
        return false;
    }

    // We Get current target size if exists
    if (target) {
        D2D1_SIZE_F size = target->GetSize();
        if (FAILED(hr)) {
            return false;
        }
        if (size.width < metrics.widthIncludingTrailingWhitespace) {
            dirty = true;
        }
        if (size.height < metrics.height) {
            dirty = true;
        }
    }

    if (dirty) {
        width = max(width,   metrics.widthIncludingTrailingWhitespace);
        height = max(height, metrics.height);

        if (!create()) {
            return false;
        }
    }

    // Draw it
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0, 0, 0)); // Clear with transparent
    target->DrawTextLayout(
        D2D1::Point2F(0, 0),
        layout,
        brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_NONE
    );

    hr = target->EndDraw();
    return SUCCEEDED(hr);
}

BTK_PRIV_END