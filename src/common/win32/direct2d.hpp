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

// For PathWinding
#include <Btk/painter.hpp>

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

// Helper class for convert direct2d path to
class ID2D1SinkToBtkSink : public ID2D1GeometrySink {
    public:
        ID2D1SinkToBtkSink(PainterPathSink *s) : sink(s) {}

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
            sink->line_to(point.x, point.y);
        }
        void AddBezier(const D2D1_BEZIER_SEGMENT *b) noexcept override {
            sink->bezier_to(
                b->point1.x, b->point1.y,
                b->point2.x, b->point2.y,
                b->point3.x, b->point3.y
            );
        }
        void AddQuadraticBezier(const D2D1_QUADRATIC_BEZIER_SEGMENT *q) noexcept override {
            sink->quad_to(
                q->point1.x, q->point1.y,
                q->point2.x, q->point2.y

            );
        }
        void AddArc(const D2D1_ARC_SEGMENT *a) noexcept override {
            BTK_LOG("AddArc(const D2D1_ARC_SEGMENT *a) not impl\n");
        }
    private:
        PainterPathSink *sink;
};

BTK_NS_END2(BTK_NAMESPACE::Win32)