#pragma once

#include <Btk/detail/macro.hpp>
#include <cstdint>
#include <cstdio>

BTK_NS_BEGIN

// Basic types

using char_t      = char;//< Utf8 char type
using uchar_t     = char32_t;//< Unicode character type
using timerid_t   = uintptr_t;//< Timer id type
using pointer_t   = void *;//< Pointer type
using cpointer_t  = const void *;//< Const pointer type
using timestamp_t = uint32_t;//< Timestamp type

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
    public:
        inline virtual ~Any() = default;
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

BTK_FLAGS_OPERATOR(Alignment, uint8_t);

// Forward declarations
class EventQueue;
class EventLoop;
class UIContext;

class Trackable;
class Object;

class Event;
class Widget;
class Style;
class Font;

class Layout;
class BoxLayout;

class IOStream;
class IODevice;

class AbstractVideoSurface;
class AbstractAudioDevice;
class AbstractWindow;

// Painting
class GraphicsContext;
class GraphicsDriver;
class GLContext;
class PixBuffer;
class Painter;
class Texture;
class Brush;
// String 
class u8string;
class u8string_view;
class StringList;
class StringRefList;
// Keyboard
enum class Key      : uint32_t;
enum class Modifier : uint16_t;

// Alias
using  any_t      = Any *;
using  widget_t   = Widget *;
using  context_t  = UIContext *;
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

BTK_NS_END