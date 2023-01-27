#pragma once

#include "build.hpp"
#include <dwrite.h>

#if defined(_MSC_VER)
#pragma comment(lib, "dwrite.lib")
#endif

// Import Direct2D For Process Path
#include "direct2d.hpp"

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

template <typename T>
class IDWritePathRenderer : public IDWriteTextRenderer {
    public:
        IDWritePathRenderer(T *sink) : sink(sink) { }

        void set_dpi(float d) {
            dpi = d;
        }


        // Inherit from IUnknown
        ULONG AddRef() {
            return 1;
        }
        ULONG Release() {
            return 0;
        }

        HRESULT QueryInterface(REFIID riid, void **ppvObject) {
            if (riid == __uuidof(IDWritePixelSnapping)) {
                *ppvObject = this;
                return S_OK;
            }
            if (riid == __uuidof(IDWriteTextRenderer)) {
                *ppvObject = this;
                return S_OK;
            }
            return E_NOINTERFACE;
        }


        // Inherit from IDWritePixelSnapping

        HRESULT IsPixelSnappingDisabled(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) {
            *isDisabled = FALSE;
            return S_OK;
        }

        HRESULT GetCurrentTransform(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) {
            static_assert(sizeof(DWRITE_MATRIX) == sizeof(D2D1_MATRIX_3X2_F));
            auto mat = D2D1::Matrix3x2F::Identity();

            Btk_memcpy(transform, &mat, sizeof(mat));
            return S_OK;
        }
        STDMETHOD(GetPixelsPerDip)(
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
        ) {
            *pixelsPerDip = 1.0f / dpi;
            return S_OK;
        }

        // Inherit from IDWriteTextRenderer

        HRESULT DrawGlyphRun(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE measuringMode,
            DWRITE_GLYPH_RUN const* glyphRun,
            DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
            IUnknown* clientDrawingEffect
        ) {
            ID2D1SinkToBtkSink cvt_sink {sink};
            glyphRun->fontFace->GetGlyphRunOutline(
                glyphRun->fontEmSize,
                glyphRun->glyphIndices,
                glyphRun->glyphAdvances,
                glyphRun->glyphOffsets,
                glyphRun->glyphCount,
                glyphRun->isSideways,
                glyphRun->bidiLevel % 2,
                &cvt_sink
            );
            return S_OK;
        }

        HRESULT DrawUnderline(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
        ) {
            BTK_LOG("IDWritePathRenderer doesnot support DrawUnderline\n");
            return E_NOINTERFACE;
        }
        
        HRESULT DrawStrikethrough(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
        ) {
            return E_NOINTERFACE;
        }


        HRESULT DrawInlineObject (
            void* clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject* inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect
        ) {
            BTK_LOG("IDWritePathRenderer doesnot support DrawInlineObject\n");
            return E_NOINTERFACE;
        }
    private:
        FLOAT dpi = 96.0f;
        T *sink;
};

BTK_NS_END2(BTK_NAMESPACE::Win32)