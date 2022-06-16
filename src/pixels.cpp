#include "build.hpp"

#include <Btk/pixels.hpp>

BTK_NS_BEGIN

PixBuffer::~PixBuffer() {
    if(_owned){
        free(_pixels);
    }
}
PixBuffer::PixBuffer() {
    _bpp = 0;
    _owned = false;
}
PixBuffer::PixBuffer(int w, int h) {
    _width = w;
    _height = h;
    _pitch = w * 4;
    _bpp = 32;
    _owned = true;
    _pixels = malloc(w * h * 4);

    // Zero out the buffer
    Btk_memset(_pixels, 0, w * h * 4);
}
PixBuffer::PixBuffer(PixBuffer &&bf) {
    Btk_memcpy(this, &bf, sizeof(PixBuffer));
    Btk_memset(&bf, 0, sizeof(PixBuffer));
}

uint32_t PixBuffer::pixel_at(int x,int y) const {
    uint8_t bytes_per_pixel = this->bytes_per_pixel();
    const uint8_t *buf = static_cast<const uint8_t*>(_pixels) + (y * _pitch) + (x * bytes_per_pixel);
    uint32_t pix = 0;
    Btk_memcpy(&pix, buf, bytes_per_pixel);
    return pix;
}

PixBuffer PixBuffer::clone() const {
    if (empty()) {
        return PixBuffer();
    }
    PixBuffer bf(_width, _height);
    Btk_memcpy(bf._pixels, _pixels, _width * _height * bytes_per_pixel());
    return bf;
}

BTK_NS_END