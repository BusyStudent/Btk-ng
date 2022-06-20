#pragma once

#include <Btk/detail/blend.hpp>
#include <Btk/rect.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class Color;
class GLColor;

class Color {
    public:
        uint8_t r; //< Red
        uint8_t g; //< Green
        uint8_t b; //< Blue
        uint8_t a; //< Alpha

        Color() : r(0), g(0), b(0), a(0) {}
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
        Color(const Color &) = default;

        operator GLColor() const noexcept;
};
class GLColor {
    public:
        float r; //< Red component (0.0f - 1.0f)
        float g; //< Green component (0.0f - 1.0f)
        float b; //< Blue component (0.0f - 1.0f)
        float a; //< Alpha component (0.0f - 1.0f)

        GLColor() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
        GLColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
        GLColor(const GLColor &) = default;

        operator Color() const noexcept;
};

enum class PixFormat : uint32_t {
    RGBA32 = 0,
    RGB24  = 1,
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
         * @brief Construct a new Pix Buffer object
         * 
         * @param w 
         * @param h 
         */
        PixBuffer(int w, int h);
        PixBuffer(PixBuffer &&);
        ~PixBuffer();

        pointer_t  pixels() noexcept {
            return _pixels;
        }
        cpointer_t pixels() const noexcept {
            return _pixels;
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

        // Configure attributes
        void set_managed(bool managed) noexcept {
            _owned = managed;
        }

        uint32_t pixel_at(int x,int y) const;
        Color    color_at(int x,int y) const;
        void     set_color(int x,int y,Color c);

        PixBuffer clone() const;

        static PixBuffer FromFile(const char *filename);
        static PixBuffer FromStream(IOStream *stream);
    private:
        pointer_t _pixels = nullptr;
        int       _width = 0;
        int       _height = 0;
        int       _pitch = 0;

        // Format
        uint8_t  _bpp   : 7; //< Bits per pixel
        uint8_t  _owned : 1; //< If true, the PixBuffer owns the pixels
        uint32_t _rmask = 0;
        uint32_t _gmask = 0;
        uint32_t _bmask = 0;
        uint32_t _amask = 0;
        
};

inline Color::operator GLColor() const noexcept {
    return GLColor{
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    };
}
inline GLColor::operator Color() const noexcept {
    return Color{
        uint8_t(r * 255.0f),
        uint8_t(g * 255.0f),
        uint8_t(b * 255.0f),
        uint8_t(a * 255.0f)
    };
}

BTK_NS_END