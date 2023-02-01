// FIXME : Outline is diff with direct2d

#pragma once

#include "build.hpp"
#include "common/win32/dwrite.hpp"
#include "common/win32/direct2d.hpp"

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
class ID2D1TransformSink final : public ID2D1GeometrySink {
    public:
        ID2D1TransformSink(const D2D1::Matrix3x2F &mat, ID2D1GeometrySink *s) : mat(mat), sink(s) { }

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
            sink->SetFillMode(m);
        }
        void SetSegmentFlags(D2D1_PATH_SEGMENT s) noexcept override {
            sink->SetSegmentFlags(s);
        }
        void BeginFigure(D2D1_POINT_2F p, D2D1_FIGURE_BEGIN f) noexcept override {
            p = p * mat;
            sink->BeginFigure(p, f);
        }
        void EndFigure(D2D1_FIGURE_END f) noexcept override {
            sink->EndFigure(f);
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
            sink->AddLine(point);
        }
        void AddBezier(const D2D1_BEZIER_SEGMENT *b) noexcept override {
            // Do transform
            auto c = *b;
            c.point1 = c.point1 * mat;
            c.point2 = c.point2 * mat;
            c.point3 = c.point3 * mat;

            sink->AddBezier(c);
        }
        void AddQuadraticBezier(const D2D1_QUADRATIC_BEZIER_SEGMENT *q) noexcept override {
            // Do transform
            auto c = *q;
            c.point1 = c.point1 * mat;
            c.point2 = c.point2 * mat;

            sink->AddQuadraticBezier(c);
        }
        void AddArc(const D2D1_ARC_SEGMENT *a) noexcept override {
            BTK_LOG("AddArc(const D2D1_ARC_SEGMENT *a) not impl\n");
        }
    private:
        const D2D1::Matrix3x2F &mat;
        ID2D1GeometrySink *sink;
};

class IDWritePathRenderer : public IDWriteTextRenderer {
    public:
        IDWritePathRenderer(PainterPath *sink, float dpi) : out(sink), dpi(dpi) { }

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
            void* clientDrawingContext,
            BOOL* isDisabled
        ) noexcept {
            *isDisabled = FALSE;
            return S_OK;
        }

        HRESULT GetCurrentTransform(
            void* clientDrawingContext,
            DWRITE_MATRIX* transform
        ) noexcept {
            static_assert(sizeof(DWRITE_MATRIX) == sizeof(D2D1_MATRIX_3X2_F));
            auto mat = D2D1::Matrix3x2F::Identity();

            Btk_memcpy(transform, &mat, sizeof(mat));
            return S_OK;
        }
        HRESULT GetPixelsPerDip (
                void* clientDrawingContext,
                FLOAT* pixelsPerDip
        ) noexcept {
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
        ) noexcept {
            // https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/DWriteColorGlyph/cpp/CustomTextRenderer.cpp
            // Transform it
            auto mat = D2D1::Matrix3x2F::Translation(
                baselineOriginX,
                height
            );
            ID2D1TransformSink trans(mat, path_sink.Get());

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
        _In_ DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
        ) noexcept {
            BTK_LOG("IDWritePathRenderer doesnot support DrawUnderline\n");
            return E_NOINTERFACE;
        }
        
        HRESULT DrawStrikethrough(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
        ) noexcept {
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
        ) noexcept {
            BTK_LOG("IDWritePathRenderer doesnot support DrawInlineObject\n");
            return E_NOINTERFACE;
        }

        // My Func
        void set_size(FLOAT w, FLOAT h) {
            width = w;
            height = h;
        }

        void begin() {
            HRESULT hr;
            hr = Direct2D::GetInstance()->CreatePathGeometry(&path_geo);
            hr = path_geo->Open(&path_sink);
        }
        void end() {
            HRESULT hr;
            hr = path_sink->Close();

            // Place at center of the box
            D2D1_RECT_F box;
            hr = path_geo->GetBounds(nullptr, &box);

            float dst_y = height / 2 - (box.bottom - box.top) / 2;
            float yoffset = -(box.top - dst_y);

            // float yoffset = 
            //     (height - box.bottom - box.top) / 2;

            out->open();

            auto mat = D2D1::Matrix3x2F::Translation(
                0,
                yoffset
            );
            IBtkTransformSink trans(mat, out);
            hr = path_geo->Stream(&trans);

            out->close();
        }
    private:
        Direct2DInitializer       init;
        ComPtr<ID2D1PathGeometry> path_geo;
        ComPtr<ID2D1GeometrySink> path_sink;
        FLOAT dpi = 96.0f;
        FLOAT height = 0.0f;
        FLOAT width  = 0.0f;
        PainterPath *out;
};

inline HRESULT DWriteGetTextLayoutOutline(IDWriteTextLayout *layout, float dpi, PainterPath *out) {
    if (!layout || !out || dpi <= 0) {
        return E_FAIL;
    }
    HRESULT hr;
    IDWritePathRenderer renderer(out, dpi);
    DWRITE_TEXT_METRICS m;
    
    hr = layout->GetMetrics(&m);
    if (FAILED(hr)) {
        return hr;
    }
    renderer.set_size(
        m.widthIncludingTrailingWhitespace,
        m.height
    );

    renderer.begin();
    hr = layout->Draw(
        nullptr,
        &renderer,
        0,
        0
    );
    renderer.end();
    
    return hr;
}

BTK_PRIV_END