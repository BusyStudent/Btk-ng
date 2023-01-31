#include "build.hpp"
#include "common/utils.hpp" //< For refcounting

#include <Btk/pixels.hpp>
#include <array>

#if  defined(_WIN32)
#include "common/win32/wincodec.hpp" //< Bulitin Wincodec wrapper
#include <wincodec.h> //< Load image from file / memory
#include <shlwapi.h> //< PathFindExtensionW
#include <cwchar> //< wcslen
#include <wrl.h>

using Microsoft::WRL::ComPtr;
// WIC Internal forward declarations
namespace {
    using namespace BTK_NAMESPACE;
    bool      wic_run_codecs(IWICBitmapDecoder *decoder, size_t frame_idx, PixBuffer *result);
    bool      wic_save_to(const PixBuffer *buffer, IStream *stream, const wchar_t *fmt);
}
#else
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC

// Replace memory fn
#define STBI_REALLOC Btk_realloc
#define STBI_MALLOC  Btk_malloc
#define STBI_FREE    Btk_free

#define STBI_WINDOWS_UTF8

#include "libs/stb_image.h" //Stb library image loading
#endif

BTK_NS_BEGIN

Color::Color(u8string_view str) {
    *this = Black;
    int ir, ig, ib, ia = 255;
    bool ok = false;

    if (str.empty()) {
        return;
    }
    else if (str.front() == '#') {
        u8string us(str);

        if (str.size() == 7) {
            // #RRGGBB
            ok = (sscanf(us.c_str() + 1, "%02x%02x%02x", &ir, &ig, &ib) == 3);
        }
        else if (str.size() == 9) {
            // #RRGGBBAA
            ok = (sscanf(us.c_str() + 1, "%02x%02x%02x%02x",  &ir, &ig, &ib, &ia) == 4);
        }
    }
    else if (str.starts_with("rgb(") && str.ends_with(")")) {
        u8string us(str);
        ok = (sscanf(us.c_str(), "rgb(%i,%i,%i)", &ir, &ig, &ib) == 3);
    }
    else if (str.starts_with("rgba(") && str.ends_with(")")) {
        u8string us(str);
        float fa;
        ok = (sscanf(us.c_str(), "rgba(%i,%i,%i,%f)", &ir, &ig, &ib, &fa) == 4);
        // Convert to uint8 range
        ia = fa * 255.0f;
    }
    if (ok) {
        this->r = ir;
        this->g = ig;
        this->b = ib;
        this->a = ia;
    }
}
PixBuffer::~PixBuffer() {
    clear();
}
PixBuffer::PixBuffer() {
    _bpp = 0;
    _owned = false;
}
PixBuffer::PixBuffer(PixFormat fmt, int w, int h) {
    _init_format(fmt);
    int byte = (_bpp + 7) / 8;

    _width = w;
    _height = h;
    _pitch = w * byte;
    _owned = true;
    _pixels = Btk_malloc(w * h * byte);

    // Zero out the buffer
    Btk_memset(_pixels, 0, w * h * byte);

    // Alloc refcount
    _refcount = new int(1);
}
PixBuffer::PixBuffer(PixFormat fmt, void *p, int w, int h) {
    _init_format(fmt);
    int byte = (_bpp + 7) / 8;

    _width = w;
    _height = h;
    _pitch = w * byte;
    _owned = false;
    _pixels = p;


    // Alloc refcount
    _refcount = new int(1);
}
PixBuffer::PixBuffer(PixBuffer &&bf) {
    Btk_memcpy(this, &bf, sizeof(PixBuffer));
    Btk_memset(&bf, 0, sizeof(PixBuffer));
}
PixBuffer::PixBuffer(const PixBuffer &bf) {
    Btk_memcpy(this, &bf, sizeof(PixBuffer));

    if (bf._refcount != nullptr) {
        ++ *bf._refcount;
    }
}
void PixBuffer::clear() {
    if (_refcount == nullptr) {
        // No ref
        return;
    }

    if (-- *_refcount > 0) {
        // Still has references
        return;
    }

    delete _refcount;
    if(_owned){
        Btk_free(_pixels);
    }

    // Zero out the buffer
    Btk_memset(this, 0, sizeof(PixBuffer));
}

// Utils for setting the pixel data
uint32_t PixBuffer::pixel_at(int x, int y) const {
    uint8_t bytes_per_pixel = this->bytes_per_pixel();
    const uint8_t *buf = static_cast<const uint8_t*>(_pixels) + (y * _pitch) + (x * bytes_per_pixel);
    uint32_t pix = 0;

    switch (bytes_per_pixel) {
        case sizeof(uint8_t) : {
            pix = *buf;
            break;
        } 
        case sizeof(uint16_t) : {
            pix = *reinterpret_cast<const uint16_t*>(buf);
            break;
        }
        case sizeof(uint32_t) : {
            pix = *reinterpret_cast<const uint32_t*>(buf);
            break;
        }
        default : {
            Btk_memcpy(&pix, buf, bytes_per_pixel);
            break;
        }
    }
    return pix;
}
Color   PixBuffer::color_at(int x, int y) const {
    uint32_t pix = pixel_at(x, y);
    Color c;
    c.r = (pix & _rmask) >> _rshift;
    c.g = (pix & _gmask) >> _gshift;
    c.b = (pix & _bmask) >> _bshift;
    c.a = (pix & _amask) >> _ashift;
    return c;
}
void   PixBuffer::set_color(int x, int y, Color c) {
    uint32_t pix = 0;
    pix |= (c.r << _rshift);
    pix |= (c.g << _gshift);
    pix |= (c.b << _bshift);
    pix |= (c.a << _ashift);
    set_pixel(x, y, pix);
}
void   PixBuffer::set_pixel(int x, int y, uint32_t pix) {
    uint8_t bytes_per_pixel = this->bytes_per_pixel();
    uint8_t *buf = static_cast<uint8_t*>(_pixels) + (y * _pitch) + (x * bytes_per_pixel);
    
    switch (bytes_per_pixel) {
        case sizeof(uint8_t) : {
            *buf = pix;
            break;
        } 
        case sizeof(uint16_t) : {
            *reinterpret_cast<uint16_t*>(buf) = pix;
            break;
        }
        case sizeof(uint32_t) : {
            *reinterpret_cast<uint32_t*>(buf) = pix;
            break;
        }
        default : {
            Btk_memcpy(buf, &pix, bytes_per_pixel);
            break;
        }
    }
}

PixBuffer PixBuffer::clone() const {
    if (empty()) {
        return PixBuffer();
    }
    PixBuffer bf(_width, _height);
    Btk_memcpy(bf._pixels, _pixels, _width * _height * bytes_per_pixel());
    return bf;
}
PixBuffer PixBuffer::resize(int w, int h) const {

#if defined(_WIN32)
    auto wic = Win32::Wincodec::GetInstance();

    ComPtr<IWICBitmapScaler> scaler;
    ComPtr<IWICFormatConverter> converter;
    HRESULT hr;

    hr = wic->CreateBitmapScaler(&scaler);
    hr = wic->CreateFormatConverter(&converter);

    Win32::IBtkBitmap source(this);
    // Scale the bitmap to the new size
    // Emm, WICBitmapInterpolationModeHighQualityCubic is undefined in MinGW :(
    hr = scaler->Initialize(&source, w, h, WICBitmapInterpolationModeCubic);

    hr = converter->Initialize(
        scaler.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom
    );

    // Create a new PixBuffer
    PixBuffer buf(PixFormat::RGBA32, w, h);

    // Copy to the new PixBuffer
    hr = converter->CopyPixels(
        nullptr,
        buf._pitch,
        buf._pitch * h,
        static_cast<BYTE*>(buf._pixels)
    );
    return buf;
#else
    PixBuffer buf(w, h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_x = _width  * (float(w) / float(x));
            int src_y = _height * (float(h) / float(y));

            buf.set_color(x, y, color_at(src_x, src_y));
        }
    }

    return buf;
#endif
}

PixBuffer PixBuffer::convert(PixFormat fmt) const {
    PixBuffer dst(fmt, _width, _height);

    // For each pixel convert to the new format
    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            dst.set_color(x, y, color_at(x, y));
        }
    }
    
    return dst;
}
PixBuffer PixBuffer::filter2d(const double *kernel, int kw, int kh, uint8_t ft_alpha) const {
    PixBuffer buf(_width, _height);
    double sums[4];
    
    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            // Zero buffer
            Btk_memset(sums, 0, sizeof(sums));

            for (int ky = 0; ky < kh; ky ++) {
                for (int kx = 0; kx < kw; kx ++) {
                    int px = x - (kw) / 2 + kx;
                    int py = y - (kh) / 2 + ky;

                    Color color;

                    // Map out of range coord
                    if (px < 0) {
                        px = -px;
                    }
                    if (py < 0) {
                        py = -py;
                    }
                    if (px >= _width) {
                        px = _width - (px - _width + 1);
                    }
                    if (py >= _height) {
                        py = _height -(py - _height + 1);
                    }
                    color = color_at(px, py);

                    sums[0] += color.r * kernel[ky * kw + kx];
                    sums[1] += color.g * kernel[ky * kw + kx];
                    sums[2] += color.b * kernel[ky * kw + kx];
                    sums[3] += color.a * kernel[ky * kw + kx];
                }
            }
            
            for (auto &value : sums) {
                value = clamp(value, 0.0, 255.0);
            }
            if (!ft_alpha) {
                sums[3] = 255.0;
            }

            Color dst = Color(sums[0], sums[1], sums[2], sums[3]);
            buf.set_color(x, y, dst);
        }
    }
    return buf;
}
PixBuffer PixBuffer::blur(float r) const {
    if (r <= 0) {
        return ref();
    }

    // Fast blur algorithm from https://blog.ivank.net/fastest-gaussian-blur.html
    PixBuffer dest(_width, _height);
    PixBuffer source = clone();

    auto make_guassion_box = [](float sigma, int nbox) -> std::array<int, 3> {
        assert(nbox == 3);

        auto ideal = std::sqrt(12 * sigma * sigma *nbox + 1);
        int  wl = std::floor(ideal);

        if (wl % 2 == 0) {
            wl -= 1;
        }

        int wu = wl + 2;

        ideal = (12 * sigma * sigma - nbox * wl * wl - 4 * nbox *wl - 3 * nbox) / (-4 * wl - 4);
        int m = std::round(ideal);

        std::array<int, 3> ret;
        for (int i = 0;i < nbox; i ++) {
            ret[i] = i < m ? wl : wu;
        }
        return ret;
    };
    auto round_color = [](int64_t sums[4], float factor) -> Color {
        return Color(
            std::round(sums[0] * factor),
            std::round(sums[1] * factor),
            std::round(sums[2] * factor),
            std::round(sums[3] * factor)
        );
    };
    auto do_boxlur_h = [&](PixBuffer &dst, const PixBuffer &src, int r) {
        float factor = 1.0f / (r + r + 1);
        for (int i = 0;i < src.height(); i++) {
            int ti = i * src.width(), li = ti, ri = ti + r;
            auto fv = src.color_at(ti, 0);
            auto lv = src.color_at(ti + src.width() - 1, 0);

            int64_t sums[4];
            sums[0] = (r + 1) * int64_t(fv.r);
            sums[1] = (r + 1) * int64_t(fv.g);
            sums[2] = (r + 1) * int64_t(fv.b);
            sums[3] = (r + 1) * int64_t(fv.a);

            for(int j = 0  ; j <  r ; j++) {
                auto c = src.color_at(ti + j, 0);
                sums[0] += c.r;
                sums[1] += c.g;
                sums[2] += c.b;
                sums[3] += c.a;
            }
            for(int j = 0  ; j <= r ; j++) {
                auto c = src.color_at(ri++, 0);
                sums[0] += c.r - fv.r;
                sums[1] += c.g - fv.g;
                sums[2] += c.b - fv.b;
                sums[3] += c.a - fv.a;

                dst.set_color(ti++, 0, round_color(sums, factor));
            }
            for(int j = r + 1; j < src.width() - r; j++) { 
                auto a = src.color_at(ri++, 0);
                auto b = src.color_at(li++, 0);

                sums[0] += int64_t(a.r) - int64_t(b.r);
                sums[1] += int64_t(a.g) - int64_t(b.g);
                sums[2] += int64_t(a.b) - int64_t(b.b);
                sums[3] += int64_t(a.a) - int64_t(b.a);

                dst.set_color(ti++, 0, round_color(sums, factor));
            }
            for(int j = src.width() - r; j < src.width()  ; j++) { 
                auto b = src.color_at(li++, 0);
                sums[0] += int64_t(lv.r) - int64_t(b.r);
                sums[1] += int64_t(lv.g) - int64_t(b.g);
                sums[2] += int64_t(lv.b) - int64_t(b.b);
                sums[3] += int64_t(lv.a) - int64_t(b.a);

                dst.set_color(ti++, 0, round_color(sums, factor));
            }

        }
    };
    auto do_boxlur_v = [&](PixBuffer &dst, const PixBuffer &src, int r) {
        float factor = 1.0f / (r + r + 1);
        for (int i = 0;i < src.width(); i++) {
            int ti = i, li = ti, ri = ti + r * src.width();
            auto fv = src.color_at(ti, 0);
            auto lv = src.color_at(ti + src.width() * (src.height() - 1), 0);

            int64_t sums[4];
            sums[0] = (r + 1) * int64_t(fv.r);
            sums[1] = (r + 1) * int64_t(fv.g);
            sums[2] = (r + 1) * int64_t(fv.b);
            sums[3] = (r + 1) * int64_t(fv.a);

            for(int j = 0  ; j <  r ; j++) {
                auto c = src.color_at(ti + j * src.width(), 0);
                sums[0] += c.r;
                sums[1] += c.g;
                sums[2] += c.b;
                sums[3] += c.a;
            }
            for(int j = 0  ; j <= r ; j++) {
                auto c = src.color_at(ri, 0);
                sums[0] += c.r - fv.r;
                sums[1] += c.g - fv.g;
                sums[2] += c.b - fv.b;
                sums[3] += c.a - fv.a;

                dst.set_color(ti, 0, round_color(sums, factor));

                ri += src.width();
                ti += src.width();
            }
            for(int j = r + 1; j < src.height() - r; j++) { 
                auto a = src.color_at(ri, 0);
                auto b = src.color_at(li, 0);

                sums[0] += int64_t(a.r) - int64_t(b.r);
                sums[1] += int64_t(a.g) - int64_t(b.g);
                sums[2] += int64_t(a.b) - int64_t(b.b);
                sums[3] += int64_t(a.a) - int64_t(b.a);

                dst.set_color(ti, 0, round_color(sums, factor));

                li += src.width();
                ri += src.width();
                ti += src.width();
            }
            for(int j = src.height() - r; j < src.height()  ; j++) { 
                auto b = src.color_at(li, 0);
                sums[0] += int64_t(lv.r) - int64_t(b.r);
                sums[1] += int64_t(lv.g) - int64_t(b.g);
                sums[2] += int64_t(lv.b) - int64_t(b.b);
                sums[3] += int64_t(lv.a) - int64_t(b.a);

                dst.set_color(ti, 0, round_color(sums, factor));

                li += src.width();
                ti += src.width();
            }
        }
    };
    auto do_boxblur = [&](PixBuffer &dst, PixBuffer &src, int r) {
        dst.bilt(src, nullptr, nullptr);
        do_boxlur_h(src, dst, r);
        do_boxlur_v(dst, src, r);
    };
    // for (int value : make_guassion_box(r, 3)) {
    //     do_boxblur(buf, *this, (value - 1) / 2);
    // }
    auto box = make_guassion_box(r, 3);
    do_boxblur(dest, source, (box[0] - 1) / 2);
    do_boxblur(source, dest, (box[1] - 1) / 2);
    do_boxblur(dest, source, (box[2] - 1) / 2);

    return dest;
}

// TODO : Optomize
void PixBuffer::bilt(const PixBuffer &buf, const Rect *_dst, const Rect *_src) {
    if (_dst == nullptr && _src == nullptr && size() == buf.size() && format() == buf.format()) {
        // All same, just memcpy
        Btk_memcpy(_pixels, buf._pixels, _width * _height * bytes_per_pixel());
        return;
    }

    Rect dst, src;
    if (_dst) {
        dst = *_dst;
    }
    else {
        dst.x = 0;
        dst.y = 0;
        dst.w = _width;
        dst.h = _height;
    }
    if (_src) {
        src = *_src;
    }
    else {
        src.x = 0;
        src.y = 0;
        src.w = buf.width();
        src.h = buf.height();
    }

    // Begin bilt
    for (int yoff = 0; yoff < dst.h; yoff++) {
        for (int xoff = 0; xoff < dst.w; xoff++) {
            int x = dst.x + xoff;
            int y = dst.y + yoff;

            // Map pixels (temp use Nearest)
            int src_x = src.x + src.w * (float(dst.w) / float(xoff));
            int src_y = src.y + src.h * (float(dst.h) / float(yoff));

            set_color(x, y, buf.color_at(src_x, src_y));
        }
    }
}

// Write to
bool PixBuffer::write_to(u8string_view path) const {
    if (empty()) {
        return false;
    }
#if defined(_WIN32)
    auto u16 = path.to_utf16();
    auto ws  = reinterpret_cast<const wchar_t*>(u16.c_str());
    ComPtr<IStream> stream;
    HRESULT hr;

    // Get ext name
    auto ext = PathFindExtensionW(ws);
    if (ext == ws + path.size()) {
        // No EXT
        return false;
    }
    ext += 1; //< Skip the dot
    // Open File
    hr = SHCreateStreamOnFileW(ws, STGM_WRITE | STGM_CREATE, &stream);
    if (FAILED(hr)) {
        return false;
    }

    return wic_save_to(this, stream.Get(), ext);
#else
    return false;
#endif
}

// Operators
PixBuffer &PixBuffer::operator =(const PixBuffer &bf) {
    clear();
    Btk_memcpy(this, &bf, sizeof(PixBuffer));

    if (bf._refcount != nullptr) {
        ++ *bf._refcount;
    }

    return *this;
}
PixBuffer &PixBuffer::operator =(PixBuffer &&bf) {
    clear();
    Btk_memcpy(this, &bf, sizeof(PixBuffer));
    Btk_memset(&bf, 0, sizeof(PixBuffer));

    return *this;
}

// Load from ...
PixBuffer PixBuffer::FromFile(u8string_view path) {

#if defined(_WIN32)
    auto wic = Win32::Wincodec::GetInstance();
    auto u16 = path.to_utf16();

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr;

    hr = wic->CreateDecoderFromFilename(
        reinterpret_cast<LPCWSTR>(u16.c_str()),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );
    if (SUCCEEDED(hr)) {
        PixBuffer result;
        if (wic_run_codecs(decoder.Get(), 0, &result)) {
            return result;
        }
    }
    return PixBuffer();
#else
    int w, h, comp;
    auto data = stbi_load(u8string(path).c_str(),&w, &h, &comp, STBI_rgb_alpha);
    if (!data) {
        return PixBuffer();
    }

    // Manage by pixbuffer
    if (comp == STBI_rgb_alpha) {
        PixBuffer buf(PixFormat::RGBA32, data, w, h);
        buf.set_managed(true);
        return buf;
    }
    else if (comp == STBI_rgb) {
        // We need to convert to RGBA32 (ABGR in little ed)
        PixBuffer buf(PixFormat::RGBA32, w, h);
        auto dst = buf.pixels<uint32_t>();

        for (int i = 0; i < w * h; i++) {
            uint32_t pix = 0xFF000000;
            uint32_t r = data[i * 4 + 0];
            uint32_t g = data[i * 4 + 1];
            uint32_t b = data[i * 4 + 2];

            pix |= b << 16;
            pix |= g << 8;
            pix |= r << 0;

            dst[i] = pix;

        }
        Btk_free(data);
        return buf;
    }
    // Unsupport
    abort();
#endif

}
PixBuffer PixBuffer::FromMem(cpointer_t data, size_t n) {

#if defined(_WIN32)
    auto wic = Win32::Wincodec::GetInstance();

    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IStream>           stream;
    HRESULT hr;

    stream = SHCreateMemStream(static_cast<const BYTE*>(data), n);

    if (stream) {
        return PixBuffer();
    }

    hr = wic->CreateDecoderFromStream(
        stream.Get(),
        nullptr,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );

    if (SUCCEEDED(hr)) {
        PixBuffer result;
        if (wic_run_codecs(decoder.Get(), 0, &result)) {
            return result;
        }
    }
    return PixBuffer();
#else
    return PixBuffer();
#endif

}

void PixBuffer::_init_format(PixFormat fmt) {
    if (fmt == PixFormat::RGBA32) {
        // Little endian ABGR
        _rmask = 0x000000FF;
        _gmask = 0x0000FF00;
        _bmask = 0x00FF0000;
        _amask = 0xFF000000;

        _rshift = 0;
        _gshift = 8;
        _bshift = 16;
        _ashift = 24;

        _bpp = 32;
    }
    else if(fmt == PixFormat::RGB24) {
        // RR GG BB
        _rmask = 0xFF00000000;
        _gmask = 0x00FF000000;
        _bmask = 0x0000FF0000;
        _amask = 0x0000000000;

        _rshift = 24;
        _gshift = 16;
        _bshift = 8;
        _ashift = 0;

        _bpp = 24;
    }
    else if (fmt == PixFormat::Gray8) {
        // Pad Pad Pad 0A
        _rmask = 0x0000000000;
        _gmask = 0x0000000000;
        _bmask = 0x0000000000;
        _amask = 0x00000000FF;

        _rshift = 0;
        _gshift = 0;
        _bshift = 0;
        _ashift = 0;

        _bpp = 8;
    } 
    _format = fmt;
}

// Image
class ImageImpl : public Refable<ImageImpl> {
    public:
        std::vector<PixBuffer> buffers  = {};
        std::vector<int>       delays   = {};
        size_t                 nframe = 0;

#if     defined(_WIN32)
        // Wincodec 
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IStream>           stream;

        // Wincodec gif compose 
        PixBuffer               cframe = {};
        int                     last = -1; //< Last decoded frame  
        bool                    gif = false;
#endif
};

// For operator =
COW_COPY_ASSIGN_IMPL(Image);
COW_MOVE_ASSIGN_IMPL(Image);

Image::Image() {
    priv = nullptr;
}
Image::Image(const Image &image) {
    priv = COW_REFERENCE(image.priv);
}
Image::Image(Image &&i) {
    priv = i.priv;
    i.priv = nullptr;
}
Image::~Image() {
    COW_RELEASE(priv);
}

size_t Image::count_frame() const {
    return priv->nframe;
}
bool   Image::read_frame(size_t idx, PixBuffer &buf, int *delay) const {
    if (idx >= priv->nframe) {
        BTK_LOG("[Image] Request frame %ld equal or bigger than %ld\n", idx, priv->nframe);
        return false;
    }
    // Get delay from cache
    if (delay) {
        *delay = priv->delays[idx];
    }

#if defined(_WIN32)
    if (priv->decoder) {
        if (!priv->gif) {
            // Not gif
            return wic_run_codecs(priv->decoder.Get(), idx, &buf);
        }
        // We need compose
        if (!wic_run_codecs(priv->decoder.Get(), idx, &buf)) {
                return false;
        }
        // First frame
        if (idx == 0) {
            priv->last = 0;
            if (!priv->cframe.empty() && priv->cframe.size() == buf.size()) {
                // Reuse
                priv->cframe.bilt(buf, nullptr, nullptr);
            }
            else {
                priv->cframe = buf.clone(); //< Cached frame
            }
            return true;
        }
        // TODO : We should decode frame by idx
        // Begin compose
        auto out = buf.pixels<uint32_t>();
        auto in  = priv->cframe.pixels<uint32_t>();
        for (int y = 0; y < buf.height(); y++) {
            for (int x = 0; x < buf.width(); x++) {
                // If pixels is trans, use previous frame data
                if (buf.color_at(x, y).a == 0) {
                    out[y * buf.width() + x] = in[y * buf.width() + x];
                }
            }
        }
        // Done
        Btk_memcpy(in, out, buf.height() * buf.pitch());
        priv->last = idx;
        return true;
    }
#else
    if (idx < priv->buffers.size()) {
        buf = priv->buffers[idx];
        return true;
    }
#endif
    return false;
}
bool Image::empty() const {
    return priv == nullptr;
}
void Image::clear() {
    COW_RELEASE(priv);
}

Image Image::FromFile(u8string_view path) {

#if defined(_WIN32)
    auto wic = Win32::Wincodec::GetInstance();
    auto u16 = path.to_utf16();

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr;
    UINT    nframe;
    GUID    container;

    hr = wic->CreateDecoderFromFilename(
        reinterpret_cast<LPCWSTR>(u16.c_str()),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );
    if (FAILED(hr)) {
        return Image();
    }
    hr = decoder->GetFrameCount(&nframe);
    if (FAILED(hr)) {
        return Image();
    }
    hr = decoder->GetContainerFormat(&container);
    if (FAILED(hr)) {
        return Image();
    }

    Image image;
    image.priv = new ImageImpl;
    image.priv->set_refcount(1); //< Default refcount to 1
    image.priv->delays.resize(nframe, -1); //< Default delay to -1 (undefined)
    image.priv->decoder = decoder;
    image.priv->nframe = nframe;
    image.priv->gif = (container == GUID_ContainerFormatGif);

    // Get delay if has mlti frame

    if (nframe > 1) {
        ComPtr<IWICMetadataQueryReader> reader;
        ComPtr<IWICBitmapFrameDecode> frame;
        PROPVARIANT variant;

        PropVariantInit(&variant);
        for (int i = 0; i < nframe; i++) {
            hr = decoder->GetFrame(i, frame.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                continue;
            }
            hr = frame->GetMetadataQueryReader(reader.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                continue;
            }
            hr = reader->GetMetadataByName(L"/grctlext/Delay", &variant);
            if (FAILED(hr)) {
                // Try webp
                hr = reader->GetMetadataByName(L"/ANMF/FrameDuration", &variant);
            }
            if (FAILED(hr)) {
                // All failed
                continue;
            }
            hr = (variant.vt == VT_UI2 ? S_OK : E_FAIL);
            if (SUCCEEDED(hr)) {
                // Got
                UINT delay;
                hr = UIntMult(variant.uiVal, 10, &delay);
                image.priv->delays[i] = SUCCEEDED(hr) ? delay : -1;
            }
            PropVariantClear(&variant);
        }
    }

    return image;
#else
    FILE *f = stbi__fopen(u8string(path).c_str(), "rb");
    if (!f) {
        return Image();
    }
    stbi__context s;
    stbi__start_file(&s,f);

    Image image;
    image.priv = new ImageImpl;
    image.priv->set_refcount(1);

    bool gif = stbi__gif_test(&s);
    if (gif) {
        int w, h, nframe, comp;
        int *delays;

        stbi_uc *frames = static_cast<stbi_uc*>(stbi__load_gif_main(&s, &delays, &w, &h, &nframe, &comp, STBI_rgb_alpha));
        if (!frames) {
            fclose(f);
            return Image();
        }
        
        image.priv->nframe = nframe;
        image.priv->delays.resize(nframe);
        image.priv->buffers.resize(nframe);
        assert(comp == STBI_rgb_alpha);

        for (int n = 0; n < nframe; n++) {
            image.priv->buffers[n] = PixBuffer(PixFormat::RGBA32, w, h);
            image.priv->delays[n] = delays[n];

            // Write frame
            Btk_memcpy(image.priv->buffers[n].pixels(), frames + n * (w * h *comp), (w * h *comp));
        }
        stbi_image_free(frames);
        stbi_image_free(delays);
    }
    else {
        // TODO : Finish it
        auto buf = PixBuffer::FromFile(path);

        if (buf.empty()) {
            fclose(f);
            return Image();
        }
        
        image.priv->nframe = 1;
        image.priv->delays.push_back(-1);
        image.priv->buffers.push_back(buf);
    }

    fclose(f);
    
    return image;
#endif
}

BTK_NS_END



#if defined(_WIN32)
// WIC Implementation
namespace {
    bool      wic_run_codecs(IWICBitmapDecoder *decoder, size_t frame_idx, PixBuffer *result) {
        auto wic = Win32::Wincodec::GetInstance();

        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        UINT width, height;
        HRESULT hr;
        
        // Create frame
        hr = decoder->GetFrame(frame_idx, frame.GetAddressOf());

        if (FAILED(hr)) {
            BTK_LOG("[Wincodec] Failed to Get frame\n");
            goto err;
        }

        // Create converter
        hr = wic->CreateFormatConverter(converter.GetAddressOf());

        if (FAILED(hr)) {
            BTK_LOG("[Wincodec] Failed to Create converter\n");
            goto err;
        }

        // Convert to 32bpp
        hr = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom
        );

        if (FAILED(hr)) {
            BTK_LOG("[Wincodec] Failed to Initialize converter\n");
            goto err;
        }

        // Create buffer
        hr = converter->GetSize(&width, &height);
        if (result->empty() || result->size() != Size(width, height)) {
            *result = PixBuffer(PixFormat::RGBA32, width, height);
        }

        hr = converter->CopyPixels(
            nullptr,
            result->pitch(),
            result->height() * result->pitch(),
            reinterpret_cast<BYTE*>(result->pixels())
        );

        return true;

        err:
            BTK_LOG("[Wincodec] Failed to decode hr = %d\n", int(hr));
            return false;
    }
    bool wic_save_to(const PixBuffer *buffer, IStream *stream, const wchar_t *ext) {
        auto wic = Win32::Wincodec::GetInstance();
        ComPtr<IWICBitmapEncoder> encoder;
        ComPtr<IWICBitmapFrameEncode> frame;

        int width  = buffer->width();
        int height = buffer->height();
        int pitch  = buffer->pitch();

        // Create encoder
        const GUID *fmt;

        // Match extension
        if (_wcsicmp(ext, L"png") == 0) {
            fmt = &GUID_ContainerFormatPng;
        } 
        else if (_wcsicmp(ext, L"jpg") == 0) {
            fmt = &GUID_ContainerFormatJpeg;
        }
        else if (_wcsicmp(ext, L"jpeg") == 0) {
            fmt = &GUID_ContainerFormatJpeg;
        }
        else if (_wcsicmp(ext, L"bmp") == 0) {
            fmt = &GUID_ContainerFormatBmp;
        }
#if defined(_MSC_VER)
        else if (_wcsicmp(ext, L"raw") == 0) {
            fmt = &GUID_ContainerFormatRaw;
        }
        else if (_wcsicmp(ext, L"webp") == 0) {
            fmt = &GUID_ContainerFormatWebp;
        }
#endif
        else if (_wcsicmp(ext, L"tiff") == 0) {
            fmt = &GUID_ContainerFormatTiff;
        }
        else {
            // Unsupported format
            return false;
        }

        HRESULT hr = wic->CreateEncoder(*fmt, nullptr, &encoder);

        if (FAILED(hr)) {
            // Failed to create encoder
            return false;
        }

        if (FAILED(hr)) {
            // Failed to create stream
            return false;
        }

        // Create frame
        hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
        if (FAILED(hr)) {
            // Failed to initialize encoder
            return false;
        }

        // Create frame
        hr = encoder->CreateNewFrame(&frame, nullptr);
        if (FAILED(hr)) {
            // Failed to create frame
            return false;
        }

        // Initialize frame
        hr = frame->Initialize(nullptr);
        if (FAILED(hr)) {
            // Failed to initialize frame
            return false;
        }

        // Set frame properties
        WICPixelFormatGUID pf = GUID_WICPixelFormat32bppRGBA;
        hr = frame->SetPixelFormat(&pf);
        if (FAILED(hr)) {
            // Failed to set pixel format
            return false;
        }

        // Set frame properties
        hr = frame->SetSize(width, height);
        if (FAILED(hr)) {
            // Failed to set size
            return false;
        }

        // Write pixels to frame
        if (pf != GUID_WICPixelFormat32bppRGBA) {
            ComPtr<IWICFormatConverter> converter;
            Win32::IBtkBitmap bmp(buffer);

            hr = wic->CreateFormatConverter(converter.GetAddressOf());
            if (FAILED(hr)) {
                // Failed to create converter
                return false;
            }
            hr = converter->Initialize(
                &bmp,
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom
            );
            hr = frame->WriteSource(converter.Get(), nullptr);
        }
        else {
            // Same format, just copy
            hr = frame->WritePixels(
                height,
                pitch, 
                pitch * height, 
                (BYTE *)buffer->pixels()
            );
        }

        if (FAILED(hr)) {
            // Failed to write pixels
            return false;
        }

        // Commit frame
        hr = frame->Commit();
        if (FAILED(hr)) {
            // Failed to commit frame
            return false;
        }

        hr = encoder->Commit();
        if (FAILED(hr)) {
            // Failed to commit encoder
            return false;
        }

        return true;
    }
}
#endif