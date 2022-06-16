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
class AnyRef {
    public:
        virtual void ref() = 0;
        virtual void unref() = 0;
};

// Forward declarations
class EventQueue;
class EventLoop;
class UIContext;

class Trackable;
class Object;

class Event;
class Widget;
class Style;

class IOStream;
class IODevice;

class AbstractTexture;
class AbstractWindow;
class AbstractFont;

// Painting
class GraphicsContext;
class GraphicsDriver;
class VGraphicsContext;
class PixBuffer;
class Painter;
class Texture;
// String 
class u8string;
class u8string_view;
// Keyboard
enum class Key      : uint32_t;
enum class Modifier : uint16_t;

// Alias
using  any_t      = Any *;
using  widget_t   = Widget *;
using  context_t  = UIContext *;
using  font_t     = AbstractFont *;
using  window_t   = AbstractWindow *;
using  texture_t  = AbstractTexture *;
using  gcontext_t = GraphicsContext *;
using vgcontext_t = VGraphicsContext *;

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