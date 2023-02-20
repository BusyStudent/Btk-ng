// FIXME : Outline is diff with direct2d

#pragma once

#include "build.hpp"
#include "common/win32/dwrite.hpp"
#include "common/win32/direct2d.hpp"

#include <Btk/painter.hpp>

#include <wrl.h>

BTK_PRIV_BEGIN

using Microsoft::WRL::ComPtr;
using Win32::Direct2D;
using Win32::Direct2DInitializer;

// Helper class for convert direct2d path to
class IBtkTransformSink final : public ID2D1GeometrySink {
    public:
        IBtkTransformSink(const D2D1::Matrix3x2F &mat, PainterPathSink *s) : mat(mat), sink(s) { }

        HRESULT QueryInterface(REFIID riid, void **lp) override {
            if (riid == __uuidof(ID2D1GeometrySink)) {
                *lp = this;
                return S_OK;
            }
            if (riid == __uuidof(ID2D1SimplifiedGeometrySink)) {
                *lp = this;
                return S_OK;
            }
            return E_NOINTERFACE;
        }
        ULONG AddRef() override {
            return 1;
        }
        ULONG Release() override {
            return 0;
        }

        // ID2D1SimplifiedGeometrySink
        void SetFillMode(D2D1_FILL_MODE m) noexcept override {
            if (m == D2D1_FILL_MODE_WINDING) {
                sink->set_winding(PathWinding::Solid);
            }
            else {
                sink->set_winding(PathWinding::Hole);
            }
        }
        void SetSegmentFlags(D2D1_PATH_SEGMENT s) noexcept override {
            BTK_LOG("SetSegmentFlags(D2D1_PATH_SEGMENT s) not impl\n");
        }
        void BeginFigure(D2D1_POINT_2F p, D2D1_FIGURE_BEGIN f) noexcept override {
            if (f == D2D1_FIGURE_BEGIN_HOLLOW) {
                BTK_LOG("f == D2D1_FIGURE_BEGIN_HOLLOW\n");
            }
            p = p * mat;
            sink->move_to(p.x, p.y);
        }
        void EndFigure(D2D1_FIGURE_END f) noexcept override {
            if (f == D2D1_FIGURE_END_CLOSED) {
                // Explict close
                sink->close_path();
            }
        }
        void AddLines(const D2D1_POINT_2F *fp, UINT32 n) noexcept override {
            for (UINT32 i = 0; i < n; i ++) {
                AddLine(fp[i]);
            }
        }
        void AddBeziers(const D2D1_BEZIER_SEGMENT *bs, UINT32 n) noexcept override {
            for (UINT32 i = 0; i < n; i ++) {
                AddBezier(&bs[i]);
            }
        }
        void AddQuadraticBeziers(const D2D1_QUADRATIC_BEZIER_SEGMENT *qs, UINT32 n) noexcept override {
            for (UINT32 i = 0; i < n; i ++) {
                AddQuadraticBezier(&qs[i]);
            }
        }
        HRESULT Close() noexcept override {
            return S_OK;
        }

        // ID2D1GeometrySink
        void AddLine(D2D1_POINT_2F point) noexcept override {
            // Do transform
            point = point * mat;
            sink->line_to(point.x, point.y);
        }
        void AddBezier(const D2D1_BEZIER_SEGMENT *b) noexcept override {
            // Do transform
            auto c = *b;
            c.point1 = c.point1 * mat;
            c.point2 = c.point2 * mat;
            c.point3 = c.point3 * mat;

            sink->bezier_to(
                c.point1.x, c.point1.y,
                c.point2.x, c.point2.y,
                c.point3.x, c.point3.y
            );
        }
        void AddQuadraticBezier(const D2D1_QUADRATIC_BEZIER_SEGMENT *q) noexcept override {
            // Do transform
            auto c = *q;
            c.point1 = c.point1 * mat;
            c.point2 = c.point2 * mat;

            sink->quad_to(
                c.point1.x, c.point1.y,
                c.point2.x, c.point2.y
            );
        }
        void AddArc(const D2D1_ARC_SEGMENT *a) noexcept override {
            BTK_LOG("AddArc(const D2D1_ARC_SEGMENT *a) not impl\n");
        }
    private:
        const D2D1::Matrix3x2F &mat;
        PainterPathSink *sink;
};

class IDWritePathRenderer final : public IDWriteTextRenderer {
    public:
        IDWritePathRenderer(PainterPath *sink, float dpi) : out(sink), dpi(dpi) { }

        // Inherit from IUnknown
        ULONG AddRef() noexcept override {
            return 1;
        }
        ULONG Release() noexcept override {
            return 0;
        }

        HRESULT QueryInterface(REFIID riid, void **ppvObject) noexcept override {
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
            void* clientDrawingContext,
            BOOL* isDisabled
        ) noexcept override {
            *isDisabled = FALSE;
            return S_OK;
        }

        HRESULT GetCurrentTransform(
            void* clientDrawingContext,
            DWRITE_MATRIX* transform
        ) noexcept override {
            static_assert(sizeof(DWRITE_MATRIX) == sizeof(D2D1_MATRIX_3X2_F));
            auto mat = D2D1::Matrix3x2F::Identity();

            Btk_memcpy(transform, &mat, sizeof(mat));
            return S_OK;
        }
        HRESULT GetPixelsPerDip (
                void* clientDrawingContext,
                FLOAT* pixelsPerDip
        ) noexcept override {
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
        ) noexcept override {
            // https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/DWriteColorGlyph/cpp/CustomTextRenderer.cpp
            // Transform it

            // Get baseline of it
            UINT count = 0;
            FLOAT baseline = 0;
            for (auto &m : metrics) {
                auto cur = glyphRunDescription->textPosition;
                if (cur >= count && cur < count + m.length) {
                    baseline += m.baseline;
                    break;
                }
                else {
                    // Next line, add height
                    baseline += m.height;
                }
                count += m.length;
            }

            auto mat = D2D1::Matrix3x2F::Translation(
                baselineOriginX,
                baseline
            );
            IBtkTransformSink trans(mat, out);

            auto hr = glyphRun->fontFace->GetGlyphRunOutline(
                glyphRun->fontEmSize,
                glyphRun->glyphIndices,
                glyphRun->glyphAdvances,
                glyphRun->glyphOffsets,
                glyphRun->glyphCount,
                glyphRun->isSideways,
                glyphRun->bidiLevel % 2,
                &trans
            );
            return S_OK;
        }

        HRESULT DrawUnderline(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_UNDERLINE const* underline,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            BTK_LOG("IDWritePathRenderer doesnot support DrawUnderline\n");
            return E_NOTIMPL;
        }
        
        HRESULT DrawStrikethrough(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_STRIKETHROUGH const* strikethrough,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            return E_NOTIMPL;
        }


        HRESULT DrawInlineObject (
            void* clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject* inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            BTK_LOG("IDWritePathRenderer doesnot support DrawInlineObject\n");
            return E_NOTIMPL;
        }

        void set_layout(IDWriteTextLayout *layout) {
            textlayout = layout;

            DWRITE_LINE_METRICS m;
            HRESULT hr;
            UINT32 count = 0;

            hr = layout->GetLineMetrics(&m, 0, &count);

            // Alloc space
            metrics.resize(count);
            hr = layout->GetLineMetrics(metrics.data(), count, &count);
        }
    private:
        std::vector<DWRITE_LINE_METRICS> metrics;
        IDWriteTextLayout *textlayout = nullptr;

        FLOAT dpi = 96.0f;
        PainterPath *out;
};
// For Raster Bitmap
class IDWriteBitmapRenderer final : public IDWriteTextRenderer {
    public:
        IDWriteBitmapRenderer(float dpi) : dpi(dpi) { 
            transform = D2D1::Matrix3x2F::Scale(
                dpi / 96.0f,
                dpi / 96.0f
            );
        }

        // Inherit from IUnknown
        ULONG AddRef() noexcept override {
            return 1;
        }
        ULONG Release() noexcept override {
            return 0;
        }

        HRESULT QueryInterface(REFIID riid, void **ppvObject) noexcept override {
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
            void* clientDrawingContext,
            BOOL* isDisabled
        ) noexcept override {
            *isDisabled = FALSE;
            return S_OK;
        }

        HRESULT GetCurrentTransform(
            void* clientDrawingContext,
            DWRITE_MATRIX* transform
        ) noexcept override {
            static_assert(sizeof(DWRITE_MATRIX) == sizeof(D2D1_MATRIX_3X2_F));
            // auto mat = D2D1::Matrix3x2F::Identity();
            auto mat = transform;

            Btk_memcpy(transform, &mat, sizeof(mat));
            return S_OK;
        }
        HRESULT GetPixelsPerDip (
                void* clientDrawingContext,
                FLOAT* pixelsPerDip
        ) noexcept override {
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
        ) noexcept override {
            // https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/DWriteColorGlyph/cpp/CustomTextRenderer.cpp
            // Transform it

            // Get baseline of it
            UINT count = 0;
            FLOAT baseline = 0;
            for (auto &m : metrics) {
                auto cur = glyphRunDescription->textPosition;
                if (cur >= count && cur < count + m.length) {
                    baseline += m.baseline;
                    break;
                }
                else {
                    // Next line, add height
                    baseline += m.height;
                }
                count += m.length;
            }

            // auto mat = D2D1::Matrix3x2F::Translation(
            //     baselineOriginX,
            //     baseline
            // );

            ComPtr<IDWriteGlyphRunAnalysis> analysis;
            auto hr = Win32::DWrite::GetInstance()->CreateGlyphRunAnalysis(
                glyphRun,
                1,
                reinterpret_cast<DWRITE_MATRIX*>(&transform),
                DWRITE_RENDERING_MODE_ALIASED,
                DWRITE_MEASURING_MODE_NATURAL,
                baselineOriginX,
                baseline,
                &analysis
            );
            if (FAILED(hr)) {
                return hr;
            }

            RECT bounds;
            hr = analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);

            Rect rect;
            rect.x = bounds.left;
            rect.y = bounds.top;
            rect.w = bounds.right - bounds.left;
            rect.h = bounds.bottom - bounds.top;

            // Alloc memory
            PixBuffer buffer(PixFormat::Gray8, rect.w, rect.h);
            hr = analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &bounds, buffer.pixels<BYTE>(), rect.w * rect.h);

            // Print
            // for (int y = 0; y < bounds.bottom - bounds.top; y++) {
            //     for (int x = 0; x < bounds.right - bounds.left; x++) {
            //         auto value = 
            //             buffer.pixel_at(x, y);
            //         if (value) {
            //             putchar('#');
            //         }
            //         else {
            //             putchar(' ');
            //         }
            //     }
            //     putchar('\n');

            // }

            if (output_buffer) {
                output_buffer->push_back(buffer);
            }
            if (output_rect) {
                output_rect->push_back(rect);
            }

            return S_OK;
        }

        HRESULT DrawUnderline(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_UNDERLINE const* underline,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            BTK_LOG("IDWritePathRenderer doesnot support DrawUnderline\n");
            return E_NOTIMPL;
        }
        
        HRESULT DrawStrikethrough(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_STRIKETHROUGH const* strikethrough,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            return E_NOTIMPL;
        }


        HRESULT DrawInlineObject (
            void* clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject* inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect
        ) noexcept override {
            BTK_LOG("IDWritePathRenderer doesnot support DrawInlineObject\n");
            return E_NOTIMPL;
        }

        void set_layout(IDWriteTextLayout *layout) {
            textlayout = layout;

            DWRITE_LINE_METRICS m;
            HRESULT hr;
            UINT32 count = 0;

            hr = layout->GetLineMetrics(&m, 0, &count);

            // Alloc space
            metrics.resize(count);
            hr = layout->GetLineMetrics(metrics.data(), count, &count);
        }
        void set_output_rect(std::vector<Rect> *o) {
            output_rect = o;
            if (o) {
                o->clear();
            }
        }
        void set_output_buffer(std::vector<PixBuffer> *o) {
            output_buffer = o;
            if (o) {
                o->clear();
            }
        }
    private:
        std::vector<Rect>               *output_rect = nullptr;
        std::vector<PixBuffer>          *output_buffer = nullptr;
        std::vector<DWRITE_LINE_METRICS> metrics;
        IDWriteTextLayout *textlayout = nullptr;

        FLOAT dpi = 96.0f;
        D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Identity();
};


inline HRESULT DWriteGetTextLayoutOutline(IDWriteTextLayout *layout, float dpi, PainterPath *out) {
    if (!layout || !out || dpi <= 0) {
        return E_FAIL;
    }
    HRESULT hr;
    IDWritePathRenderer renderer(out, dpi);

    renderer.set_layout(layout);
    
    out->open();
    hr = layout->Draw(
        nullptr,
        &renderer,
        0,
        0
    );
    out->close();
    
    return hr;
}

inline HRESULT DWriteGetTextLayoutBitmap(IDWriteTextLayout *layout, float dpi, std::vector<PixBuffer> *buf, std::vector<Rect> *rect) {
    IDWriteBitmapRenderer renderer(dpi);
    renderer.set_layout(layout);
    renderer.set_output_buffer(buf);
    renderer.set_output_rect(rect);

    return layout->Draw(nullptr, &renderer, 0, 0);
}

BTK_PRIV_END