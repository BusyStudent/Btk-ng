#pragma once

#include "build.hpp"
#include <Btk/pixels.hpp>
#include <wincodec.h>

#if defined(_MSC_VER)
#pragma comment(lib, "windowscodecs.lib")
#endif

BTK_NS_BEGIN2(BTK_NAMESPACE::Win32)

class Wincodec {
    private:
        inline static IWICImagingFactory *factory = nullptr;
    public:
        static void   Init() {
            if (factory) {
                factory->AddRef();
                return;
            }
            HRESULT hr;
            // Init Com
            hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            BTK_ASSERT(SUCCEEDED(hr));

            hr = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory)
            );
            BTK_ASSERT(SUCCEEDED(hr));
        }
        static void    Shutdown() {
            if (!factory) {
                return;
            }
            if (factory->Release() == 0) {
                factory = nullptr;
                CoUninitialize();
            }
        }
        static auto    GetInstance() -> IWICImagingFactory *{
            return factory;
        }
};
class WincodecInitializer {
    public:
        WincodecInitializer() {
            Wincodec::Init();
        }
        ~WincodecInitializer() {
            Wincodec::Shutdown();
        }
};

/**
 * @brief Wrap PixBuffer to IWICBitmapSource
 * 
 */
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


BTK_NS_END2(BTK_NAMESPACE::Win32)