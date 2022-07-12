#include "build.hpp"

#include <Btk/painter.hpp>
#include <Btk/pixels.hpp>

#if defined(_WIN32)
#include <wincodec.h> //< Load image from file / memory
#include <shlwapi.h> //< PathFindExtensionW
#include <cwchar> //< wcslen
#include <wrl.h>

using Microsoft::WRL::ComPtr;
#endif

BTK_NS_BEGIN

PixBuffer::~PixBuffer() {
    clear();
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
        free(_pixels);
    }

    // Zero out the buffer
    Btk_memset(this, 0, sizeof(PixBuffer));
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

// Write to
void PixBuffer::write_to(const char_t *path) const {
    if (empty()) {
        return;
    }
#if defined(_WIN32)
    auto u16 = u8string_view(path).to_utf16();
    auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;

    // TODO : 

    // Get file extension
    LPWSTR ext = PathFindExtensionW(reinterpret_cast<LPCWSTR>(u16.c_str()));

    if (ext == (LPWSTR)u16.c_str() + u16.length()) {
        // Not found
        return;
    }
    ext += 1; //< Skip '.'

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
    else {
        // Unsupported format
        return;
    }

    HRESULT hr = wic->CreateEncoder(*fmt, nullptr, &encoder);

    if (FAILED(hr)) {
        // Failed to create encoder
        return;
    }

    // Create IStream based on file path
    ComPtr<IStream> stream;
    hr = SHCreateStreamOnFileW(reinterpret_cast<LPCWSTR>(u16.c_str()), STGM_WRITE | STGM_CREATE, &stream);

    if (FAILED(hr)) {
        // Failed to create stream
        return;
    }

    // Create frame
    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) {
        // Failed to initialize encoder
        return;
    }

    // Create frame
    hr = encoder->CreateNewFrame(&frame, nullptr);
    if (FAILED(hr)) {
        // Failed to create frame
        return;
    }

    // Initialize frame
    hr = frame->Initialize(nullptr);
    if (FAILED(hr)) {
        // Failed to initialize frame
        return;
    }

    // Set frame properties
    WICPixelFormatGUID pf = GUID_WICPixelFormat32bppRGBA;
    hr = frame->SetPixelFormat(&pf);
    if (FAILED(hr)) {
        // Failed to set pixel format
        return;
    }

    // Set frame properties
    hr = frame->SetSize(_width, _height);
    if (FAILED(hr)) {
        // Failed to set size
        return;
    }

    // Write pixels to frame
    hr = frame->WritePixels(_height, _pitch, _height * _pitch, static_cast<BYTE*>(_pixels));

    if (FAILED(hr)) {
        // Failed to write pixels
        return;
    }

    // Commit frame
    hr = frame->Commit();
    if (FAILED(hr)) {
        // Failed to commit frame
        return;
    }
#else

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
PixBuffer PixBuffer::FromFile(const char *s) {

#if defined(_WIN32)
    auto wic = static_cast<IWICImagingFactory*>(Win32::WicFactory());

    // Create decoder
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    UINT width, height;
    HRESULT hr;

    PixBuffer bf;
    auto u16 = u8string_view(s).to_utf16();

    hr = wic->CreateDecoderFromFilename(
        reinterpret_cast<const wchar_t*>(u16.c_str()),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        decoder.GetAddressOf()
    );

    if (FAILED(hr)) {
        goto err;
    }

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
#else
    return PixBuffer();
#endif

}


BTK_NS_END