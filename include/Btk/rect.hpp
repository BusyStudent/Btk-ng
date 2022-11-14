#pragma once

#include <Btk/defs.hpp>
#include <iosfwd>
#include <cmath>

#if !defined(_MSC_VER)
#include <ostream>
#endif

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

        // Check if two points are equal.
        bool compare(const PointImpl &p) const {
            return x == p.x && y == p.y;
        }
        bool operator ==(const PointImpl &p) const {
            return compare(p);
        }
        bool operator !=(const PointImpl &p) const {
            return !compare(p);
        }

        // Vector operations
        PointImpl operator +(const PointImpl &p) const {
            return PointImpl(x + p.x, y + p.y);
        }
        PointImpl operator -(const PointImpl &p) const {
            return PointImpl(x - p.x, y - p.y);
        }
        PointImpl operator *(double s) const {
            // Using double to avoid rounding errors.
            return PointImpl(x * s, y * s);
        }
        PointImpl operator /(double s) const {
            return PointImpl(x / s, y / s);
        }
        PointImpl operator -() const {
            return PointImpl(-x, -y);
        }
        PointImpl &operator +=(const PointImpl &p) {
            x += p.x;
            y += p.y;
            return *this;
        }
        PointImpl &operator -=(const PointImpl &p) {
            x -= p.x;
            y -= p.y;
            return *this;
        }
        PointImpl &operator *=(double s) {
            x *= s;
            y *= s;
            return *this;
        }
        PointImpl &operator /=(double s) {
            x /= s;
            y /= s;
            return *this;
        }

        PointImpl  normalized() const {
            T len = length();
            return PointImpl(x / len, y / len);
        }

        T          operator *(const PointImpl &p) const {
            return x * p.x + y * p.y;
        }
        T          operator ^(const PointImpl &p) const {
            return x * p.y - y * p.x;
        }

        auto       length() const { return std::sqrt(x * x + y * y); }
        auto       length2() const { return x * x + y * y; }

        // Cast

        template <typename Elem>
        PointImpl<Elem> cast() const {
            return PointImpl<Elem>(static_cast<Elem>(x), static_cast<Elem>(y));
        }
        template <typename Elem>
        operator PointImpl<Elem>() const {
            return PointImpl<Elem>(static_cast<Elem>(x), static_cast<Elem>(y));
        }
};

template <typename T>
class SizeImpl {
    public:
        T w, h;

        SizeImpl() = default;
        SizeImpl(T w, T h) : w(w), h(h) {}
        SizeImpl(const SizeImpl &s) : w(s.w), h(s.h) {}
        // Allow implicit conversion from PointImpl.
        template <typename Elem>
        SizeImpl(const SizeImpl<Elem> &s) : w(static_cast<T>(s.w)), h(static_cast<T>(s.h)) {}

        bool is_valid() const {
            return w > 0 && h > 0;
        }
        bool compare(const SizeImpl &s) const {
            return w == s.w && h == s.h;
        }
        bool operator ==(const SizeImpl &s) const {
            return compare(s);
        }
        bool operator !=(const SizeImpl &s) const {
            return !compare(s);
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
        bool     empty() const noexcept {
            return w <= 0 || h <= 0;
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
                w - margin.left - margin.right,
                h - margin.top  - margin.bottom
            );
        }

        template <typename Elem>
        RectImpl<Elem> unapply_margin(Elem margin) const {
            return RectImpl<Elem>(
                x - margin,
                y - margin,
                w + margin * 2,
                h + margin * 2
            );
        }
        template <typename Elem>
        RectImpl<Elem> unapply_margin(const MarginImpl<Elem> &margin) const {
            return RectImpl<Elem>(
                x - margin.left,
                y - margin.top,
                w + margin.left + margin.right,
                h + margin.top + margin.bottom
            );
        }

        // Align calc
        template <typename Elem>
        RectImpl<Elem> apply_align(const RectImpl<Elem> &box, Alignment alig) const {
            RectImpl<Elem> result = box;

            // Horizontal
            if (uint8_t(alig & Alignment::Left)) {
                result.w = w;
            }
            else if (uint8_t(alig & Alignment::Right)) {
                result.x = result.x + result.w - w;
                result.w = w;
            }
            else if (uint8_t(alig & Alignment::Center)) {
                result.x = result.x + result.w / 2 - w / 2;
                result.w = w;
            }

            // Vertical
            if (uint8_t(alig & Alignment::Top)) {
                result.h = h;
            }
            else if (uint8_t(alig & Alignment::Bottom)) {
                result.y = result.y + result.h - h;
                result.h = h;
            }
            else if (uint8_t(alig & Alignment::Middle)) {
                result.y = result.y + result.h / 2 - h / 2;
                result.h = h;
            }

            return result;
        }

        // Get
        SizeImpl<T>  size() const {
            return SizeImpl<T>(w, h);
        }
        PointImpl<T> position() const {
            return PointImpl<T>(x, y);
        }
        
        // Get detail position
        PointImpl<T> top_left() const {
            return PointImpl<T>(x, y);
        }
        PointImpl<T> top_right() const {
            return PointImpl<T>(x + w, y);
        }
        PointImpl<T> bottom_left() const {
            return PointImpl<T>(x, y + h);
        }
        PointImpl<T> bottom_right() const {
            return PointImpl<T>(x + w, y + h);
        }
        PointImpl<T> center() const {
            return PointImpl<T>(x + w / 2, y + h / 2);
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
        bool operator !=(const RectImpl &r) const {
            return !compare(r);
        }

        // Auto cast
        template <typename Elem>
        operator RectImpl<Elem>() const {
            return cast<Elem>();
        }
};

template <typename T>
class Matrix3x2Impl {
    public:
        T m[3][2] = {
            {T(1), T(0)},
            {T(0), T(1)},
            {T(0), T(0)}
        };

        Matrix3x2Impl() = default;
        Matrix3x2Impl(
            T m11, T m12,
            T m21, T m22,
            T dx, T dy
        ) {
            m[0][0] = m11; m[0][1] = m12;
            m[1][0] = m21; m[1][1] = m22;
            m[2][0] = dx;  m[2][1] = dy;
        }
        Matrix3x2Impl(const Matrix3x2Impl &m) {
            this->m[0][0] = m.m[0][0]; this->m[0][1] = m.m[0][1];
            this->m[1][0] = m.m[1][0]; this->m[1][1] = m.m[1][1];
            this->m[2][0] = m.m[2][0]; this->m[2][1] = m.m[2][1];
        }

        // Get
        const T *operator [](size_t i) const {
            return m[i];
        }
        T *operator [](size_t i) {
            return m[i];
        }

        // Calculate
        void translate(T dx, T dy) {
            m[2][0] += dx;
            m[2][1] += dy;
        }
        void scale(T sx, T sy) {
            m[0][0] *= sx;
            m[1][1] *= sy;
        }
        void scale(T s) {
            m[0][0] *= s;
            m[1][1] *= s;
        }
        void rotate(T rad) {
            T c = std::cos(rad);
            T s = std::sin(rad);
            T m11 = m[0][0] * c - m[1][0] * s;
            T m12 = m[0][1] * c - m[1][1] * s;
            T m21 = m[1][0] * c + m[0][0] * s;
            T m22 = m[1][1] * c + m[0][1] * s;
            m[0][0] = m11; m[0][1] = m12;
            m[1][0] = m21; m[1][1] = m22;
        }
        void skew(T sx, T sy) {
            m[0][1] += m[0][0] * sy;
            m[1][0] += m[1][1] * sx;
        }
        void skew_x(T sx) {
            m[0][1] += m[0][0] * sx;
        }
        void skew_y(T sy) {
            m[1][0] += m[1][1] * sy;
        }
        bool invert() {
            T det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
            if (det == 0) {
                return false;
            }
            T inv_det = 1 / det;
            T m11 = m[1][1] * inv_det;
            T m12 = -m[0][1] * inv_det;
            T m21 = -m[1][0] * inv_det;
            T m22 = m[0][0] * inv_det;
            m[0][0] = m11; m[0][1] = m12;
            m[1][0] = m21; m[1][1] = m22;
            return true;
        }
        bool invertible() const {
            T det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
            return det != 0;
        }

        Matrix3x2Impl operator *(const Matrix3x2Impl &m) const {
            Matrix3x2Impl ret;
            ret.m[0][0] = m.m[0][0] * this->m[0][0] + m.m[0][1] * this->m[1][0];
            ret.m[0][1] = m.m[0][0] * this->m[0][1] + m.m[0][1] * this->m[1][1];
            ret.m[1][0] = m.m[1][0] * this->m[0][0] + m.m[1][1] * this->m[1][0];
            ret.m[1][1] = m.m[1][0] * this->m[0][1] + m.m[1][1] * this->m[1][1];
            ret.m[2][0] = m.m[2][0] * this->m[0][0] + m.m[2][1] * this->m[1][0] + this->m[2][0];
            ret.m[2][1] = m.m[2][0] * this->m[0][1] + m.m[2][1] * this->m[1][1] + this->m[2][1];
            return ret;
        }

        template <typename Elem>
        PointImpl<Elem> operator *(const PointImpl<Elem> &p) const {
            return PointImpl<Elem>(
                p.x * m[0][0] + p.y * m[1][0] + m[2][0],
                p.x * m[0][1] + p.y * m[1][1] + m[2][1]
            );
        }

        template <typename Elem>
        Matrix3x2Impl<Elem> cast() const {
            return Matrix3x2Impl<Elem>(
                static_cast<Elem>(m[0][0]),
                static_cast<Elem>(m[0][1]),
                static_cast<Elem>(m[1][0]),
                static_cast<Elem>(m[1][1]),
                static_cast<Elem>(m[2][0]),
                static_cast<Elem>(m[2][1])
            );
        }

        // Compare
        bool compare(const Matrix3x2Impl &m) const {
            return Btk_memcmp(this, &m, sizeof(m)) == 0;
        }
        bool operator ==(const Matrix3x2Impl &m) const {
            return compare(m);
        }
        bool operator !=(const Matrix3x2Impl &m) const {
            return !compare(m);
        }

        // Helper
        static Matrix3x2Impl<T> Identity() {
            return {};
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

using FMatrix = Matrix3x2Impl<float>;

// Output templates

template <typename T>
std::ostream &operator <<(std::ostream &os, const PointImpl<T> &p) {
    return os << "Point(" << p.x << ", " << p.y << ")";
}
template <typename T>
std::ostream &operator <<(std::ostream &os, const SizeImpl<T> &s) {
    return os << "Size(" << s.w << ", " << s.h << ")";
}
template <typename T>
std::ostream &operator <<(std::ostream &os, const RectImpl<T> &r) {
    return os << "Rect(" << r.x << ", " << r.y << ", " << r.w << ", " << r.h << ")";
}
template <typename T>
std::ostream &operator <<(std::ostream &os, const MarginImpl<T> &m) {
    return os << "Margin(" << m.left << ", " << m.top << ", " << m.right << ", " << m.bottom << ")";
}
template <typename T>
std::ostream &operator <<(std::ostream &os, const Matrix3x2Impl<T> &m) {
    return os << "Matrix3x2(" << m.m[0][0] << ", " << m.m[0][1] << ", " << m.m[1][0] << ", " << m.m[1][1] << ", " << m.m[2][0] << ", " << m.m[2][1] << ")";
}

// Lerp template
template <typename T>
constexpr SizeImpl<T> lerp(const SizeImpl<T> &a, const SizeImpl<T> &b, float t) noexcept {
    return SizeImpl(
        lerp(a.w, b.w, t),
        lerp(a.h, b.h, t)
    );
}
template <typename T>
constexpr RectImpl<T> lerp(const RectImpl<T> &a, const RectImpl<T> &b, float t) noexcept {
    return RectImpl(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.w, b.w, t),
        lerp(a.h, b.h, t)
    );
}
template <typename T>
constexpr PointImpl<T> lerp(const PointImpl<T> &a, const PointImpl<T> &b, float t) noexcept {
    return PointImpl(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t)
    );
}

BTK_NS_END