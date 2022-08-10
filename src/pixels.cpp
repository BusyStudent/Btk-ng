#include "build.hpp"

#include <Btk/painter.hpp>
#include <Btk/pixels.hpp>

#if  defined(_WIN32)
#include <wincodec.h> //< Load image from file / memory
#include <shlwapi.h> //< PathFindExtensionW
#include <cwchar> //< wcslen
#include <wrl.h>

using Microsoft::WRL::ComPtr;
// WIC Internal forward declarations
namespace {
    using namespace BTK_NAMESPACE;

    IWICBitmapSource *wic_wrap(const PixBuffer *buffer);
    PixBuffer wic_run_codecs(IWICBitmapDecoder *decoder);
    bool      wic_save_to(const PixBuffer *buffer, IStream *stream, const wchar_t *fmt);
}
#else
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC

// Replace memory fn
#define STBI_REALLOC Btk_realloc
#define STBI_MALLOC  Btk_malloc
#define STBI_FREE    Btk_free

#include "libs/stb_image.h" //Stb library image loading
#endif

BTK_NS_BEGIN

PixBuffer::~PixBuffer() {
    clear();
}
PixBuffer::PixBuffer() {
    _bpp = 0;
    _owned = false;
}
PixBuffer::PixBuffer(PixFormat fmt, int w, int h) {
    int bpp = fmt == PixFormat::RGBA32 ? 32 : 24;
    int byte = fmt == PixFormat::RGBA32 ? 4 : 3;

    _width = w;
    _height = h;
    _pitch = w * byte;
    _bpp = bpp;
    _owned = true;
    _pixels = Btk_malloc(w * h * byte);

    // Zero out the buffer
    Btk_memset(_pixels, 0, w * h * byte);

    // Alloc refcount
    _refcount = new int(1);
}
PixBuffer::PixBuffer(PixFormat fmt, void *p, int w, int h) {
    int bpp = fmt == PixFormat::RGBA32 ? 32 : 24;
    int byte = fmt == PixFormat::RGBA32 ? 4 : 3;

    _width = w;
    _height = h;
    _pitch = w * byte;
    _bpp = bpp;
    _owned = false;
    _pixels = p;

    // Set RGBA mask (little endian)
    _amask = 0xFF000000;
    _gmask = 0x00FF0000;
    _bmask = 0x0000FF00;
    _rmask = 0x000000FF;

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

uint32_t PixBuffer::pixel_at(int x, int y) const {
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
PixBuffer PixBuffer::resize(int w, int h) const {

#if defined(_WIN32)
    auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());
    ComPtr<IWICBitmapScaler> scaler;
    wic->CreateBitmapScaler(&scaler);

    // Copy our bitmap to a WIC bitmap
    ComPtr<IWICBitmap> wic_bitmap;
    wic->CreateBitmapFromMemory(
        _width, _height,
        GUID_WICPixelFormat32bppPRGBA, 
        _pitch, 
        _pitch * _height, 
        static_cast<BYTE*>(_pixels),
        &wic_bitmap
    );

    // Scale the bitmap to the new size
    scaler->Initialize(wic_bitmap.Get(), w, h, WICBitmapInterpolationModeHighQualityCubic);

    // Create a new PixBuffer
    PixBuffer buf(w, h);

    // Copy to the new PixBuffer
    scaler->CopyPixels(
        nullptr,
        buf._pitch,
        buf._pitch * h,
        static_cast<BYTE*>(buf._pixels)
    );
    return buf;
#else
    return PixBuffer();
#endif
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
    auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());
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
        return wic_run_codecs(decoder.Get());
    }
    return PixBuffer();
#else
    return PixBuffer();
#endif

}
PixBuffer PixBuffer::FromMem(cpointer_t data, size_t n) {

#if defined(_WIN32)
    auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());

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
        return wic_run_codecs(decoder.Get());
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


BTK_NS_END

// WIC Implementation
namespace {
    class IBtkBitmap : public IWICBitmapSource {
        public:
            ULONG   AddRef() override { return 1; }
            ULONG   Release() override { return 1; }
            HRESULT QueryInterface(REFIID riid, void** ppvObject) override {
                if (riid == __uuidof(IWICBitmapSource)) {
                    *ppvObject = static_cast<IWICBitmapSource*>(this);
                    return S_OK;
                }
                return E_NOINTERFACE;
            }

            // Begin IWICBitmapSource
            HRESULT GetSize(UINT *w, UINT *h) override {
                assert(w && h);
                *w = buffer->width();
                *h = buffer->height();
                return S_OK;
            }
            HRESULT GetPixelFormat(GUID *fmt) override {
                assert(fmt);
                *fmt = GUID_WICPixelFormat32bppRGBA;
                return S_OK;
            }
            HRESULT GetResolution(double *x, double *y) override {
                assert(x && y);
                *x = *y = 1.0;
                return S_OK;
            }
            HRESULT CopyPalette(IWICPalette *) override {
                return E_NOTIMPL;
            }
            HRESULT CopyPixels(const WICRect *rect, UINT stride, UINT size, BYTE *ut) override {
                int x, y, w, h;
                if (rect) {
                    x = rect->X;
                    y = rect->Y;
                    w = rect->Width;
                    h = rect->Height;
                }
                else {
                    x = 0;
                    y = 0;
                    w = buffer->width();
                    h = buffer->height();
                }

                // Begin copy
                auto src = static_cast<const BYTE *>(buffer->pixels());
                auto bytes_per = buffer->bytes_per_pixel();

                for (int i = 0; i < h; ++i) {
                    auto dst = ut + i * stride;
                    auto src_ = src + (y + i) * buffer->width() * bytes_per + x * bytes_per;
                    Btk_memcpy(dst, src_, w * bytes_per);
                }
                return S_OK;
            }

            IBtkBitmap(const PixBuffer *buffer) : buffer(buffer) {}
        private:
            const PixBuffer *buffer;
    };
    PixBuffer wic_run_codecs(IWICBitmapDecoder *decoder) {
        auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());

        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        UINT width, height;
        PixBuffer bf;
        HRESULT hr;
        
        // Create frame
        hr = decoder->GetFrame(0, frame.GetAddressOf());

        if (FAILED(hr)) {
            goto err;
        }

        // Create converter
        hr = wic->CreateFormatConverter(converter.GetAddressOf());

        if (FAILED(hr)) {
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
            goto err;
        }

        // Create buffer
        hr = converter->GetSize(&width, &height);
        bf = PixBuffer(width, height);

        hr = converter->CopyPixels(
            nullptr,
            bf.pitch(),
            bf.height() * bf.pitch(),
            reinterpret_cast<BYTE*>(bf.pixels())
        );

        return bf;

        err:
            return PixBuffer();
    }
    bool wic_save_to(const PixBuffer *buffer, IStream *stream, const wchar_t *ext) {
        auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());
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
        else if (_wcsicmp(ext, L"raw") == 0) {
            fmt = &GUID_ContainerFormatRaw;
        }
        else if (_wcsicmp(ext, L"webp") == 0) {
            fmt = &GUID_ContainerFormatWebp;
        }
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
        hr = frame->WritePixels(
            height,
            pitch, 
            pitch * height, 
            (BYTE *)buffer->pixels()
        );

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
        return true;
    }
    IWICBitmapSource *wic_wrap(const PixBuffer *bf) {
        return new IBtkBitmap(bf);
    }
}