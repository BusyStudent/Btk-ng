#pragma once

#include <Btk/pixels.hpp>
#include <Btk/rect.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN
class PixBuffer;

//< Brush for painting
enum class   BrushType : uint8_t {
    Solid, //< Solid color
    Gradient, //< Gradient
    Pattern, //< Image pattern
};
class BTKAPI Brush {
    public:
        Brush();
        Brush(GLColor color);
        Brush(PixBuffer &&buf);
        Brush(Brush &&);
        ~Brush();

        void clear();
        void assign(Brush &&b);
    private:
        PixBuffer *_pixbuffer= {};
        GLColor    _color    = {};

        uint8_t    _pixowned = false;
        BrushType  _type     = BrushType::Solid;
};

class BTKAPI Texture {
    public:
        Texture() = default;
        Texture(const Texture &) = delete;
        Texture(Texture && t) {
            tex = t.tex;
            t.tex = nullptr;
        }
        ~Texture();

        void clear();
        void update(cpointer_t pix, int x, int y, int w, int h, int stride);
        void update(const PixBuffer &buffer, int x, int y);

        Size size() const;
    private:
        texture_t tex = nullptr;
};

BTK_NS_END