#pragma once

#include "build.hpp"
#include <Btk/pixels.hpp>
#include <dwrite.h>
#include <wrl.h>

BTK_NS_BEGIN2()

using Microsoft::WRL::ComPtr;
using BTK_NAMESPACE::Color;

class DWriteRaster : public IDWriteTextRenderer {
    public:
        DWriteRaster(IDWriteFactory *factory, int width, int height);
        ~DWriteRaster();

        bool draw(IDWriteTextLayout *layout, float x, float y);

        // Overrides
        HRESULT DrawGlyphRun(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE measuringMode,
            DWRITE_GLYPH_RUN const* glyphRun,
            DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
            IUnknown* clientDrawingEffect
        ) override;
    private:
        ComPtr<IDWriteBitmapRenderTarget> target;
        ComPtr<IDWriteRenderingParams>    params;
        Color                             color = Color::Black;
};

inline DWriteRaster::DWriteRaster(IDWriteFactory *factory, int width, int height) {
    ComPtr<IDWriteGdiInterop> interop;
    HRESULT hr = factory->GetGdiInterop(&interop);
    if (FAILED(hr)) {
        // Should not failed
    }
    hr = interop->CreateBitmapRenderTarget(
        CreateCompatibleDC(nullptr),
        width,
        height,
        &target
    );
}

inline bool DWriteRaster::draw(IDWriteTextLayout *layout, float x, float y) {
    HRESULT hr = layout->Draw(
        this,
        this,
        x, y
    );
    return SUCCEEDED(hr);
}

inline HRESULT DWriteRaster::DrawGlyphRun(
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
)
{
    // Forward to GDI 
    RECT dirty = {0};

    return target->DrawGlyphRun(
        baselineOriginX,
        baselineOriginY,
        measuringMode,
        glyphRun,
        params.Get(),
        RGB(color.r, color.g, color.b),
        &dirty
    );
}

BTK_NS_END2()