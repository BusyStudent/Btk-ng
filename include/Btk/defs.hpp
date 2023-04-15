#pragma once

#include <Btk/detail/macro.hpp>
#include <type_traits>
#include <cstdint>
#include <cstdio>
#include <cmath>

BTK_NS_BEGIN

// Basic types

using char_t      = char;         //< Utf8 char type
using uchar_t     = char32_t;     //< Unicode character type
using timerid_t   = uintptr_t;    //< Timer id type
using coord_t     = int;          //< Default widget coord units
using pointer_t   = void *;       //< Pointer type
using cpointer_t  = const void *; //< Const pointer type
using timestamp_t = uintptr_t;    //< Timestamp type

#if defined(_WIN32)
using ssize_t = int64_t;
#endif

// Passing to ...

using varg_bool_t   = int;
using varg_int_t    = int;
using varg_float_t  = double;
using varg_double_t = double;

/**
 * @brief Base class for all objects.
 * 
 */
class Any {
    private:
        template <typename T>
        using result_t  = std::conditional_t<std::is_pointer_v<T>, T, T&>;  // if T a pointer
        template <typename T>
        using cresult_t = std::conditional_t<std::is_pointer_v<T>, const T, const T&>;  // if T a pointer

        template <typename T>
        auto cast_base() noexcept {
            if constexpr(std::is_pointer_v<T>) {
                return this;
            }
            else {
                return static_cast<Any&>(*this);
            }
        }
        template <typename T>
        auto cast_base() const noexcept {
            if constexpr(std::is_pointer_v<T>) {
                return this;
            }
            else {
                return static_cast<const Any&>(*this);
            }
        }
    public:
        /**
         * @brief Destroy the Any object
         * 
         */
        inline virtual ~Any() = default;

        /**
         * @brief Dynmaic Cast to type T, a excpetion will be thrown if invalid.
         * 
         * @tparam T 
         * @return T& 
         */
        template <typename T>
        inline result_t<T>  as() BTK_NOEXCEPT {
#if        !defined(BTK_NO_RTTI)
            return dynamic_cast<result_t<T>>(cast_base<T>());
#else
            return static_cast<result_t<T>>(cast_base<T>());
#endif
        }
        /**
         * @brief Dynmaic Cast to type T, a excpetion will be thrown if invalid.
         * 
         * @tparam T 
         * @return const T& 
         */
        template <typename T>
        inline cresult_t<T> as() const BTK_NOEXCEPT {
#if        !defined(BTK_NO_RTTI)
            return dynamic_cast<cresult_t<T>>(cast_base<T>());
#else
            return static_cast<cresult_t<T>>(cast_base<T>());
#endif
        }
        /**
         * @brief Static Cast to type T
         * 
         * @tparam T 
         * @return T& 
         */
        template <typename T>
        inline result_t<T> unsafe_as() noexcept {
            return static_cast<result_t<T>>(cast_base<T>());
        }
        /**
         * @brief Static Cast to type T
         * 
         * @tparam T 
         * @return T& 
         */
        template <typename T>
        inline cresult_t<T> unsafe_as() const noexcept {
            return static_cast<cresult_t<T>>(cast_base<T>());
        }

        /**
         * @brief Check is inherited a class
         * 
         * @tparam T 
         * @return true 
         * @return false 
         */
        template <typename T>
        inline bool inherits() const noexcept {
            return as<T*>();
        }
    protected:
        inline Any()            = default;
        inline Any(const Any &) = default;
};

// Basic enumerations
enum class Alignment : uint8_t {
    Left     = 1 << 0,
    Right    = 1 << 1,
    Center   = 1 << 2,
    Top      = 1 << 3,
    Bottom   = 1 << 4,
    Middle   = 1 << 5,

    Baseline = 1 << 6, //< Only for text
    
};
enum class Orientation : uint8_t {
    Horizontal = 0,
    Vertical   = 1,
};
enum class Direction : uint8_t {
    LeftToRight = 0,
    RightToLeft = 1,
    TopToBottom = 2,
    BottomToTop = 3,
};
enum class TimerType : uint8_t {
    Precise = 0, 
    Coarse  = 1, //< In windows, use SetTimer()
};

constexpr inline auto Horizontal = Orientation::Horizontal;
constexpr inline auto Vertical   = Orientation::Vertical;

constexpr inline auto LeftToRight = Direction::LeftToRight;
constexpr inline auto RightToLeft = Direction::RightToLeft;
constexpr inline auto TopToBottom = Direction::TopToBottom;
constexpr inline auto BottomToTop = Direction::BottomToTop;

constexpr inline auto AlignLeft   = Alignment::Left;   //< H part
constexpr inline auto AlignRight  = Alignment::Right;
constexpr inline auto AlignCenter = Alignment::Center;
constexpr inline auto AlignTop    = Alignment::Top;    //< V part
constexpr inline auto AlignBottom = Alignment::Bottom;
constexpr inline auto AlignMiddle = Alignment::Middle;

BTK_FLAGS_OPERATOR(Orientation, uint8_t);
BTK_FLAGS_OPERATOR(Alignment,   uint8_t);

// Forward declarations
class EventDispatcher;
class EventQueue;
class EventLoop;
class UIContext;
class Event;

class Trackable;
class Object;

class Palette;
class Widget;
class Style;
class Font;

class Layout;
class BoxLayout;

class IOStream;
class IODevice;

class AbstractVideoSurface;
class AbstractAudioDevice;
class AbstractAnimation;
class AbstractWindow;
class AbstractCursor;
class AbstractScreen;

// Service
class DesktopService;

// Painting
class GraphicsDriverInfo;
class GraphicsContext;
class GraphicsDriver;
class PaintResource;
class PaintContext;
class PaintDevice;
class PainterPath;
class TextLayout;
class GLContext;
class PixBuffer;
class Painter;
class Texture;
class Brush;
class Pen;

// String 
class u8string;
class u8string_view;
class StringList;
class StringRefList;

// Keyboard
enum class Key      : uint32_t;
enum class Modifier : uint16_t;

// Mouse
enum class SystemCursor : uint32_t;
enum class MouseButton  : uint32_t;

// PixFormat
enum class PixFormat   : uint32_t;

// Signal
template <typename T>
class Signal;

// Refs
template <typename T>
class Refable;
template <typename T>
class Ref;

// Alias
using  any_t      = Any *;
using  object_t   = Object *;
using  widget_t   = Widget *;
using  context_t  = UIContext *;
using  cursor_t   = AbstractCursor *;
using  screen_t   = AbstractScreen *;
using  window_t   = AbstractWindow *;
using  driver_t   = GraphicsDriver *;

using  align_t       = Alignment;
using  timertype_t   = TimerType;
using  direction_t   = Direction;
using  orientation_t = Orientation;

// Function
BTKAPI timestamp_t GetTicks();

// Helper functions template
template <typename T>
constexpr T max(T a, T b) BTK_NOEXCEPT_IF(a > b ? a : b) {
    return a > b ? a : b;
}

template <typename T>
constexpr T min(T a, T b) BTK_NOEXCEPT_IF(a < b ? a : b) {
    return a < b ? a : b;
}

template <typename T>
constexpr T lerp(T a, T b, float t) BTK_NOEXCEPT_IF(a + (b - a) * t) {
    return a + (b - a) * t;
}

template <typename T>
constexpr T clamp(T value, T min, T max) BTK_NOEXCEPT_IF(value < min ? min : value > max ? max : value) {
    return value < min ? min : value > max ? max : value;
}

template <typename T>
constexpr T muldiv(T v, T num, T den) BTK_NOEXCEPT_IF(den == 0 ? -1 : std::round(double(v) * num / den)) {
    return den == 0 ? -1 : std::round(double(v) * num / den);
}

template <typename T, typename T1>
constexpr bool bittest(T src, T1 test) BTK_NOEXCEPT_IF(src & test) {
    using Result = decltype(src & test);
    if constexpr (std::is_enum_v<Result>) {
        return static_cast<std::underlying_type_t<Result>>(src & test);
    }
    else {
        return src & test;
    }
}

BTK_NS_END