#pragma once

#include <Btk/detail/blend.hpp>
#include <Btk/rect.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class Color;
class GLColor;

class Color {
    public:
        // Color enums packed as RRGGBBAA in 32-bit unsigned integer
        enum Enum : uint32_t {
            Red     = 0xFF0000FF,
            Green   = 0x00FF00FF,
            Blue    = 0x0000FFFF,
            Black   = 0x000000FF,
            White   = 0xFFFFFFFF,
            Purple  = 0xFF00FFFF,
            Yellow  = 0xFFFF00FF,
            Cyan    = 0x00FFFFFF,
            Magenta = 0xFF00FFFF,
            Gray    = 0x808080FF,
            Transparent = 0x00000000,
        };
    public:
        uint8_t r; //< Red
        uint8_t g; //< Green
        uint8_t b; //< Blue
        uint8_t a; //< Alpha

        constexpr Color() : r(0), g(0), b(0), a(0) {}
        constexpr Color(Enum e) : r(e >> 24), g(e >> 16), b(e >> 8), a(e & 0xFF) {}
        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
        constexpr Color(const Color &) = default;

        constexpr operator GLColor() const noexcept;
};
class GLColor {
    public:
        float r; //< Red component (0.0f - 1.0f)
        float g; //< Green component (0.0f - 1.0f)
        float b; //< Blue component (0.0f - 1.0f)
        float a; //< Alpha component (0.0f - 1.0f)

        constexpr GLColor() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
        constexpr GLColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
        constexpr GLColor(Color::Enum e) : GLColor(Color(e)) {}
        constexpr GLColor(const GLColor &) = default;

        constexpr operator Color() const noexcept;
};

enum class InterpolationMode : uint32_t {
    Nearest = 0,
    Linear  = 1,
};
enum class PixFormat         : uint32_t {
    RGBA32 = 0, //< ABGR in little ed
    RGB24  = 1,
    BGRA32 = 2,
    BGR24  = 3,
    Gray8  = 4, //< Grayscale
};

/**
 * @brief Palette for index based image
 * 
 */
class BTKAPI PixPalette {
    public:
        PixPalette();
        ~PixPalette();

    private:
        // std::vector<Color> maps;
};
/**
 * @brief RGBA Pixel Buffer
 * 
 */
class BTKAPI PixBuffer {
    public:
        /**
         * @brief Construct a new Pix Buffer object
         * 
         */
        PixBuffer();
        /**
         * @brief Construct a new RGBA32 Pix Buffer object
         * 
         * @param w 
         * @param h 
         */
        PixBuffer(int w, int h);
        /**
         * @brief Construct a new Pix Buffer object with format
         * 
         * @param w 
         * @param h 
         */
        PixBuffer(PixFormat fmt, int w, int h);
        /**
         * @brief Construct a new Pix Buffer object with format and data
         * 
         * @param p 
         * @param w 
         * @param h 
         */
        PixBuffer(PixFormat fmt, void *p, int w, int h);
        /**
         * @brief Construct a new Pix Buffer object (just reference to another buffer)
         * 
         */
        PixBuffer(const PixBuffer &);
        /**
         * @brief Construct a new Pix Buffer object (move from another buffer)
         * 
         */
        PixBuffer(PixBuffer &&);
        ~PixBuffer();
        /**
         * @brief Release contained memory
         * 
         */
        void clear();

        pointer_t  pixels() noexcept {
            return _pixels;
        }
        cpointer_t pixels() const noexcept {
            return _pixels;
        }

        template <typename T>
        T * pixels() noexcept {
            return reinterpret_cast<T *>(_pixels);
        }
        template <typename T>
        const T * pixels() const noexcept {
            return reinterpret_cast<const T *>(_pixels);
        }

        int w() const noexcept {
            return _width;
        }
        int h() const noexcept {
            return _height;
        }
        int width() const noexcept {
            return _width;
        }
        int height() const noexcept {
            return _height;
        }
        int pitch() const noexcept {
            return _pitch;
        }
        int bpp() const noexcept {
            return _bpp;
        }
        int bytes_per_pixel() const noexcept {
            return (_bpp + 7) / 8;
        }
        bool empty() const noexcept {
            return _width == 0 || _height == 0;
        }
        Size size() const noexcept {
            return Size(_width, _height);
        }
        PixFormat format() const noexcept {
            return _format;
        }

        // Configure attributes
        void set_managed(bool managed) noexcept {
            _owned = managed;
        }

        uint32_t pixel_at(int x, int y) const;
        Color    color_at(int x, int y) const;
        void     set_color(int x, int y, Color c);
        void     set_pixel(int x, int y, uint32_t c);

        // Transform / Filtering / Etc...
        template <int W, int H>
        PixBuffer filter2d(const double (&kerel)[W][H], uint8_t ft_alpha = 0) const;
        PixBuffer filter2d(const double  *kernel, int w, int h, uint8_t ft_alpha = 0) const;
        PixBuffer convert(PixFormat f) const;
        PixBuffer resize(int w, int h) const;
        PixBuffer blur(float r) const;
        PixBuffer clone() const;
        PixBuffer ref()   const;

        // Draw / Fill / Bilt
        void      bilt(const PixBuffer &buf, const Rect *dst, const Rect *src, const FMatrix &trans);
        void      bilt(const PixBuffer &buf, const Rect *dst, const Rect *src);
        void      fill(const Rect *dst, uint32_t pixel);

        // Write to file / memory / iostream
        bool      write_to(u8string_view path) const;

        // Assignment
        PixBuffer &operator =(const PixBuffer &);
        PixBuffer &operator =(PixBuffer &&);

        static PixBuffer FromFile(u8string_view path);
        static PixBuffer FromMem(cpointer_t data, size_t size);
        static PixBuffer FromStream(IOStream *stream);
    private:
        // Format initalization
        void _init_format(PixFormat fmt);

        pointer_t _pixels = nullptr;
        int       _width = 0;
        int       _height = 0;
        int       _pitch = 0;

        // Format
        PixFormat _format = {}; //< PixelFormat
        uint8_t   _bpp   : 7; //< Bits per pixel
        uint8_t   _owned : 1; //< If true, the PixBuffer owns the pixels

        // Mask / Shift
        uint32_t _rmask = 0;
        uint32_t _gmask = 0;
        uint32_t _bmask = 0;
        uint32_t _amask = 0;

        uint32_t _rshift = 0;
        uint32_t _gshift = 0;
        uint32_t _bshift = 0;
        uint32_t _ashift = 0;

        // Refcounting for COW
        int      *_refcount = nullptr;
};

/**
 * @brief Image, It can has many frames
 * 
 */
class BTKAPI Image {
    //         
};


// Helpful color functions

constexpr inline Color::operator GLColor() const noexcept {
    return GLColor{
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    };
}
constexpr inline GLColor::operator Color() const noexcept {
    return Color{
        uint8_t(r * 255.0f),
        uint8_t(g * 255.0f),
        uint8_t(b * 255.0f),
        uint8_t(a * 255.0f)
    };
}

constexpr inline Color   lerp(Color a, Color b, float t) noexcept {
    return Color{
        // Using short to avoid overflow in the subtraction
        uint8_t(lerp<short>(a.r, b.r, t)),
        uint8_t(lerp<short>(a.g, b.g, t)),
        uint8_t(lerp<short>(a.b, b.b, t)),
        uint8_t(lerp<short>(a.a, b.a, t))
    };
}

constexpr inline GLColor lerp(GLColor a, GLColor b, float t) noexcept {
    return GLColor{
        lerp(a.r, b.r, t),
        lerp(a.g, b.g, t),
        lerp(a.b, b.b, t),
        lerp(a.a, b.a, t)
    };
}

constexpr inline bool operator ==(const Color &a, const Color &b) noexcept {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
constexpr inline bool operator !=(const Color &a, const Color &b) noexcept {
    return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
}
constexpr inline bool operator ==(const GLColor &a, const GLColor &b) noexcept {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
constexpr inline bool operator !=(const GLColor &a, const GLColor &b) noexcept {
    return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
}


// PixBuffer inline functions

inline PixBuffer::PixBuffer(int w, int h) : 
    PixBuffer(PixFormat::RGBA32, w, h)
{
    
}

inline PixBuffer PixBuffer::ref() const {
    return *this;
}

template <int W, int H>
inline PixBuffer PixBuffer::filter2d(const double (&kernel)[W][H], uint8_t ft_alpha) const {
    double k[W * H];

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            k[y * W + x] = kernel[y][x];
        }
    }

    return filter2d(k, W, H, ft_alpha);
}

BTK_NS_END