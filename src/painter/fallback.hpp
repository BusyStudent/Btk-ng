#pragma once

#include "build.hpp"

#include <Btk/painter.hpp>
#include <cmath>


// Painter Fallback inline function 

BTK_NS_BEGIN
namespace {

    //
    // Copyright (c) 2013 Mikko Mononen memon@inside.org
    //
    // This software is provided 'as-is', without any express or implied
    // warranty.  In no event will the authors be held liable for any damages
    // arising from the use of this software.
    // Permission is granted to anyone to use this software for any purpose,
    // including commercial applications, and to alter it and redistribute it
    // freely, subject to the following restrictions:
    // 1. The origin of this software must not be misrepresented; you must not
    //    claim that you wrote the original software. If you use this software
    //    in a product, an acknowledgment in the product documentation would be
    //    appreciated but is not required.
    // 2. Altered source versions must be plainly marked as such, and must not be
    //    misrepresented as being the original software.
    // 3. This notice may not be removed or altered from any source distribution.
    //
    /**
     * @brief Fallback for no arc to backend
     * 
     * @param sink 
     * @param last_x 
     * @param last_y 
     * @param x1 
     * @param y1 
     * @param x2 
     * @param y2 
     * @param radius 
     */
    void fallback_painter_arc_to(
        PainterPathSink *sink, 
        float last_x, float last_y, 
        float x1, float y1, float x2, float y2, float radius
    ) {

        #define NVG_CW 0
        #define NVG_CCW 1

    #if defined(M_PI)
        #define NVG_PI M_PI
    #else
        #define NVG_PI 3.14159265358979323846
    #endif
        
        float distTol = 0.01;

        float x0 = last_x;
        float y0 = last_y;
        float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
        int dir;

        auto nvg__ptEquals = [](float x1, float y1, float x2, float y2, float tol) {
            float dx = x2 - x1;
            float dy = y2 - y1;
            return dx*dx + dy*dy < tol*tol;
        };
        auto nvg__distPtSeg = [](float x, float y, float px, float py, float qx, float qy) {
            float pqx, pqy, dx, dy, d, t;
            pqx = qx-px;
            pqy = qy-py;
            dx = x-px;
            dy = y-py;
            d = pqx*pqx + pqy*pqy;
            t = pqx*dx + pqy*dy;
            if (d > 0) t /= d;
            if (t < 0) t = 0;
            else if (t > 1) t = 1;
            dx = px + t*pqx - x;
            dy = py + t*pqy - y;
            return dx*dx + dy*dy;
        };
        auto nvg__normalize = [](float *x, float* y) {
            float d = std::sqrt((*x)*(*x) + (*y)*(*y));
            if (d > 1e-6f) {
                float id = 1.0f / d;
                *x *= id;
                *y *= id;
            }
            return d;
        };
        auto nvg__cross = [](float dx0, float dy0, float dx1, float dy1) { return dx1*dy0 - dx0*dy1; };
        auto nvgArc = [sink](float cx, float cy, float r, float a0, float a1, int dir) {
            float a = 0, da = 0, hda = 0, kappa = 0;
            float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
            float px = 0, py = 0, ptanx = 0, ptany = 0;
            float vals[3 + 5*7 + 100];
            int i, ndivs, nvals;
            // int move = NVG_LINETO;

            // Clamp angles
            da = a1 - a0;
            if (dir == NVG_CW) {
                if (std::abs(da) >= NVG_PI*2) {
                    da = NVG_PI*2;
                } else {
                    while (da < 0.0f) da += NVG_PI*2;
                }
            } else {
                if (std::abs(da) >= NVG_PI*2) {
                    da = -NVG_PI*2;
                } else {
                    while (da > 0.0f) da -= NVG_PI*2;
                }
            }

            // Split arc into max 90 degree segments.
            ndivs = max<int>(1, min<int>((int)(std::abs(da) / (NVG_PI*0.5f) + 0.5f), 5));
            hda = (da / (float)ndivs) / 2.0f;
            kappa = std::abs(4.0f / 3.0f * (1.0f - std::cos(hda)) / std::sin(hda));

            if (dir == NVG_CCW)
                kappa = -kappa;

            nvals = 0;
            for (i = 0; i <= ndivs; i++) {
                a = a0 + da * (i/(float)ndivs);
                dx = std::cos(a);
                dy = std::sin(a);
                x = cx + dx*r;
                y = cy + dy*r;
                tanx = -dy*r*kappa;
                tany = dx*r*kappa;

                if (i == 0) {
                    // vals[nvals++] = (float)move;
                    // vals[nvals++] = x;
                    // vals[nvals++] = y;
                    sink->move_to(x, y);
                } else {
                    // vals[nvals++] = NVG_BEZIERTO;
                    // vals[nvals++] = px+ptanx;
                    // vals[nvals++] = py+ptany;
                    // vals[nvals++] = x-tanx;
                    // vals[nvals++] = y-tany;
                    // vals[nvals++] = x;
                    // vals[nvals++] = y;
                    sink->bezier_to(
                        px+ptanx,
                        py+ptany,
                        x-tanx,
                        y-tany,
                        x,
                        y
                    );
                }
                px = x;
                py = y;
                ptanx = tanx;
                ptany = tany;
            }

            // nvg__appendCommands(ctx, vals, nvals);
        };

        // Handle degenerate cases.
        if (nvg__ptEquals(x0,y0, x1,y1, distTol) ||
            nvg__ptEquals(x1,y1, x2,y2, distTol) ||
            nvg__distPtSeg(x1,y1, x0,y0, x2,y2) < distTol*distTol ||
            radius < distTol) {
            
            sink->line_to(x1, y1);
            return;
        }

        // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
        dx0 = x0-x1;
        dy0 = y0-y1;
        dx1 = x2-x1;
        dy1 = y2-y1;
        nvg__normalize(&dx0,&dy0);
        nvg__normalize(&dx1,&dy1);
        a = std::acos(dx0*dx1 + dy0*dy1);
        d = radius / std::tan(a/2.0f);

    //	printf("a=%f° d=%f\n", a/NVG_PI*180.0f, d);

        if (d > 10000.0f) {
            sink->line_to(x1, y1);
            return;
        }

        if (nvg__cross(dx0,dy0, dx1,dy1) > 0.0f) {
            cx = x1 + dx0*d + dy0*radius;
            cy = y1 + dy0*d + -dx0*radius;
            a0 = std::atan2(dx0, -dy0);
            a1 = std::atan2(-dx1, dy1);
            dir = NVG_CW;
    //		printf("CW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/NVG_PI*180.0f, a1/NVG_PI*180.0f);
        } else {
            cx = x1 + dx0*d + -dy0*radius;
            cy = y1 + dy0*d + dx0*radius;
            a0 = std::atan2(-dx0, dy0);
            a1 = std::atan2(dx1, -dy1);
            dir = NVG_CCW;
    //		printf("CCW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/NVG_PI*180.0f, a1/NVG_PI*180.0f);
        }

        nvgArc(cx, cy, radius, a0, a1, dir);

        #undef NVG_CCW
        #undef NVG_CW
        #undef NVG_PI

    }

    void fallback_painter_quad_to(PainterPathSink *sink, float last_x, float last_y, float cx, float cy, float x, float y) {
        float x0 = last_x;
        float y0 = last_y;

        sink->bezier_to(
            x0 + 2.0f/3.0f*(cx - x0), 
            y0 + 2.0f/3.0f*(cy - y0),
            x + 2.0f/3.0f*(cx - x), 
            y + 2.0f/3.0f*(cy - y),
            x, 
            y 
        );
    }
}
BTK_NS_END