#pragma once

#include <Btk/defs.hpp>
#include <cmath>

BTK_NS_BEGIN

template <typename T>
class MarginImpl {
    public:
        T left;
        T right;
        T top;
        T bottom;

        MarginImpl() : left(0), right(0), top(0), bottom(0) {}
        MarginImpl(T v) : left(v), right(v), top(v), bottom(v) {}
        MarginImpl(T l, T r, T t, T b) : left(l), right(r), top(t), bottom(b) {}
        MarginImpl(const MarginImpl &) = default;

};

template <typename T>
class PointImpl {
    public:
        T x, y;

        PointImpl() = default;
        PointImpl(T x, T y) : x(x), y(y) {}
        PointImpl(const PointImpl &p) : x(p.x), y(p.y) {}

        bool compare(const PointImpl &p) const {
            return x == p.x && y == p.y;
        }
        bool operator ==(const PointImpl &p) const {
            return compare(p);
        }
};

template <typename T>
class SizeImpl {
    public:
        T w, h;

        SizeImpl() = default;
        SizeImpl(T w, T h) : w(w), h(h) {}
        SizeImpl(const SizeImpl &s) : w(s.w), h(s.h) {}

        bool is_valid() const {
            return w > 0 && h > 0;
        }
        bool compare(const SizeImpl &s) const {
            return w == s.w && h == s.h;
        }
        bool operator ==(const SizeImpl &s) const {
            return compare(s);
        }
};

template <typename T>
class RectImpl {
    public:
        T x, y, w, h;

        RectImpl() = default;
        RectImpl(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
        RectImpl(const RectImpl &r) : x(r.x), y(r.y), w(r.w), h(r.h) {}

        // Calculate

        RectImpl united(const RectImpl &r) noexcept {
            RectImpl ret;
            ret.x = min(x, r.x);
            ret.y = min(y, r.y);
            ret.w = max(x + w, r.x + r.w) - ret.x;
            ret.h = max(y + h, r.y + r.h) - ret.y;
            return ret;
        }
        RectImpl intersected(const RectImpl &r) noexcept {
            RectImpl ret;
            ret.x = max(x, r.x);
            ret.y = max(y, r.y);
            ret.w = min(x + w, r.x + r.w) - ret.x;
            ret.h = min(y + h, r.y + r.h) - ret.y;
            return ret;
        }
        bool     is_intersected(const RectImpl &r) const noexcept {
            return !(x >= r.x + r.w || r.x >= x + w || y >= r.y + r.h || r.y >= y + h);
        }

        // Cast

        template <typename Elem>
        RectImpl<Elem> cast() const {
            return RectImpl<Elem>(
                static_cast<Elem>(x),
                static_cast<Elem>(y),
                static_cast<Elem>(w),
                static_cast<Elem>(h)
            );
        }

        // Margin Calculate

        template <typename Elem>
        RectImpl<Elem> apply_margin(Elem margin) const {
            return RectImpl<Elem>(
                x + margin,
                y + margin,
                w - margin * 2,
                h - margin * 2
            );
        }
        template <typename Elem>
        RectImpl<Elem> apply_margin(const MarginImpl<Elem> &margin) const {
            return RectImpl<Elem>(
                x + margin.left,
                y + margin.top,
                w - margin.left + margin.right,
                h - margin.top + margin.bottom
            );
        }

        // Get
        SizeImpl<T> size() const {
            return SizeImpl<T>(w, h);
        }
        PointImpl<T> position() const {
            return PointImpl<T>(x, y);
        }

        // Check
        bool compare(const RectImpl &r) const {
            return x == r.x && y == r.y && w == r.w && h == r.h;
        }
        bool contains(const PointImpl<T> &p) const {
            return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
        }
        bool contains(T px, T py) const {
            return px >= x && py >= y && px < x + w && py < y + h;
        }

        bool operator ==(const RectImpl &r) const {
            return compare(r);
        }
};

using Rect  = RectImpl<int>;
using FRect = RectImpl<float>;

using Size  = SizeImpl<int>;
using FSize = SizeImpl<float>;

using Point  = PointImpl<int>;
using FPoint = PointImpl<float>;

using Margin  = MarginImpl<int>;
using FMargin = MarginImpl<float>;

BTK_NS_END