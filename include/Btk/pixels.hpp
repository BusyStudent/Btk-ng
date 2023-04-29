#pragma once

#include <Btk/rect.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class Color;
class GLColor;
class ImageImpl;

class Color {
    public:
        // Color enums packed as RRGGBBAA in 32-bit unsigned integer
        enum Enum : uint32_t {
            AliceBlue    = 0xF0F8FFFF,
            Aqua         = 0x00FFFFFF,
            Black        = 0x000000FF,
            Blue         = 0x0000FFFF,
            Brown        = 0xA52A2AFF,
            Coral        = 0xFF7F50FF,
            Cyan         = 0x00FFFFFF,
            DarkGray     = 0xA9A9A9FF,
            Fuchsia      = 0xFF00FFFF,
            Gold         = 0xFFD700FF,
            Gray         = 0x808080FF,
            Green        = 0x00FF00FF,
            Indigo       = 0x4B0082FF,
            Ivory        = 0xFFFFF0FF,
            Khaki        = 0xF0E68CFF,
            Lavender     = 0xE6E6FAFF,
            LightGray    = 0xD3D3D3FF,
            LightRed     = 0xFFC0C0FF,
            LightGreen   = 0x90EE90FF,
            LightBlue    = 0xADD8E6FF,
            LightYellow  = 0xFFFFE0FF,
            LightPink    = 0xFFB6C1FF,
            LightCyan    = 0xE0FFFFFF,
            Lime         = 0x00FF00FF,
            Magenta      = 0xFF00FFFF,
            Maroon       = 0x800000FF,
            Navy         = 0x000080FF,
            Olive        = 0x808000FF,
            Orange       = 0xFFA500FF,
            Pink         = 0xFFC0CBFF,
            Purple       = 0xFF00FFFF,
            Red          = 0xFF0000FF,
            Salmon       = 0xFA8072FF,
            Silver       = 0xC0C0C0FF,
            Tan          = 0xD2B48CFF,
            Teal         = 0x008080FF,
            Tomato       = 0xFF6347FF,
            Transparent  = 0x00000000,
            Turquoise    = 0x40E0D0FF,
            White        = 0xFFFFFFFF,
            Yellow       = 0xFFFF00FF,
        };
    public:
        uint8_t r; //< Red
        uint8_t g; //< Green
        uint8_t b; //< Blue
        uint8_t a; //< Alpha

        BTKAPI    Color(u8string_view view);

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

        BTKAPI    GLColor(u8string_view view);

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
    

    // < YUV 
    NV12   = 1001,
    NV21   = 1002,
};
// Blend mode / Bend Factor
enum class BlendMode         : uint32_t {
    None = 0,
    Alpha = 1 << 0,
    Add = 1 << 1,
    Subtract = 1 << 2,
    Modulate = 1 << 3,
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

        /**
         * @brief Get pixel data pointer
         * 
         * @return pointer_t 
         */
        pointer_t  pixels() noexcept {
            return _pixels;
        }
        /**
         * @brief Get const pixel data pointer
         * 
         * @return cpointer_t 
         */
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

        /**
         * @brief Get width of the pixbuffer
         * 
         * @return int 
         */
        int w() const noexcept {
            return _width;
        }
        /**
         * @brief Get the height of the pixbuffer
         * 
         * @return int 
         */
        int h() const noexcept {
            return _height;
        }
        /**
         * @brief Get width of the pixbuffer
         * 
         * @return int 
         */
        int width() const noexcept {
            return _width;
        }
        /**
         * @brief Get the height of the pixbuffer
         * 
         * @return int 
         */
        int height() const noexcept {
            return _height;
        }
        /**
         * @brief Get the pitch of the pixbuffer
         * 
         * @return int 
         */
        int pitch() const noexcept {
            return _pitch;
        }
        /**
         * @brief Get bits per pixel
         * 
         * @return int 
         */
        int bpp() const noexcept {
            return _bpp;
        }
        /**
         * @brief Get bytes per pixel
         * 
         * @return int 
         */
        int bytes_per_pixel() const noexcept {
            return (_bpp + 7) / 8;
        }
        /**
         * @brief Check the pixel buffer is empty
         * 
         * @return true 
         * @return false 
         */
        bool empty() const noexcept {
            return _width == 0 || _height == 0;
        }
        /**
         * @brief Get the Size(width, height) of the pixel buffer
         * 
         * @return Size 
         */
        Size size() const noexcept {
            return Size(_width, _height);
        }
        /**
         * @brief Get the format of the pixel buffer
         * 
         * @return PixFormat 
         */
        PixFormat format() const noexcept {
            return _format;
        }

        /**
         * @brief Tell the pixbuffer should he free the pixel data
         * 
         * @param managed true on free, false on no-op
         */
        void set_managed(bool managed) noexcept {
            _owned = managed;
        }

        /**
         * @brief Locate pixel at x, y (out of range on undefined-op)
         * 
         * @param x 
         * @param y 
         * @return uint32_t 
         */
        uint32_t pixel_at(int x, int y) const;
        /**
         * @brief Locate color x, y (out of range on undefined-op)
         * 
         * @param x 
         * @param y 
         * @return Color 
         */
        Color    color_at(int x, int y) const;
        /**
         * @brief Store color at x, y (out of range on undefined-op)
         * 
         * @param x 
         * @param y 
         * @param c The color object
         */
        void     set_color(int x, int y, Color c);
        /**
         * @brief Store pixel at x, y (out of range on undefined-op)
         * 
         * @param x 
         * @param y 
         * @param c pixel at pixbuffer format
         */
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
        bool      write_to(IOStream   *stream) const;

        // Assignment
        PixBuffer &operator =(const PixBuffer &);
        PixBuffer &operator =(PixBuffer &&);

        /**
         * @brief Load pixbuffer from file
         * 
         * @param path The utf8 encoded filesystem path
         * @return PixBuffer (failed on empty pixel buffer)
         */
        static PixBuffer FromFile(u8string_view path);
        /**
         * @brief Load pixbuffer from memory
         * 
         * @param data const pointer for buffer begin
         * @param size The buffer size
         * @return PixBuffer (failed on empty pixel buffer)
         */
        static PixBuffer FromMem(cpointer_t data, size_t size);
        /**
         * @brief Load pixbuffer from IOStream
         * 
         * @param stream pointer of IOStream
         * @return PixBuffer (failed on empty pixel buffer) 
         */
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
 * @brief Image, can get metadata & frame here
 * 
 */
class BTKAPI Image {
    public:
        Image();
        Image(PixBuffer   &);
        Image(const Image &);
        Image(Image &&);
        ~Image();

        /**
         * @brief Count how many frame
         * 
         * @return size_t 
         */
        size_t count_frame() const;
        /**
         * @brief Read frame at idx
         * 
         * @param idx The frame index
         * @param buf The output frame buffer
         * @param delay The pointer of frame delay (nullptr on no-op)
         * @return true 
         * @return false 
         */
        bool   read_frame(size_t idx, PixBuffer &buf, int *delay = nullptr) const;
        /**
         * @brief Is the Image empty?
         * 
         * @return true 
         * @return false 
         */
        bool   empty() const;
        /**
         * @brief Clear the image object
         * 
         */
        void   clear();

        Image &operator =(const Image &);
        Image &operator =(Image &&);

        static Image FromFile(u8string_view path);
        static Image FromMem(cpointer_t data, size_t size);
        static Image FromStream(IOStream *stream);
    private:
        ImageImpl *priv;
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