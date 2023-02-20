#include "build.hpp"
#include "dwrite_outline.hpp"
#include "common/utils.hpp"
#include "common/device_resource.hpp"
#include "common/win32/dwrite.hpp"
#include "common/win32/direct2d.hpp"

#undef min
#undef max

#include <Btk/painter.hpp>
#include <limits>

#include <wrl.h>

BTK_NS_BEGIN

using Microsoft::WRL::ComPtr;
using Win32::DWrite;

class FontImpl : public PaintResourceManager, public Refable <FontImpl> {
    public:
        IDWriteTextFormat *lazy_eval();

        ComPtr<IDWriteTextFormat> format;
        u8string                  name; //< Family name
        float                     size    = 0; //< Size in points
        DWRITE_FONT_WEIGHT        weight  = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE         style   = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH       stretch = DWRITE_FONT_STRETCH_NORMAL;
};
class TextLayoutImpl : public PaintResourceManager, public Refable <TextLayoutImpl> {
    public:
        IDWriteTextLayout *lazy_eval();
        PainterPath        outline(float dpi);

        ComPtr<IDWriteTextLayout> layout; //< Layout of the text, created on need
        Ref<FontImpl>             font;   //< Font of the text
        u8string                  text;   //< Text to render
        float                     max_width = std::numeric_limits<float>::max(); //< Max width of the text
        float                     max_height = std::numeric_limits<float>::max(); //< Max height of the text
        
        // Cached field
        FSize                     cached_size = {-1.0f, -1.0f};
        PainterPath               cached_path = { };
        float                     cached_path_dpi = 0.0f;
};

// FontImpl
inline auto FontImpl::lazy_eval() -> IDWriteTextFormat * {
    if (format == nullptr) {
        // Empty , create one
        HRESULT hr = DWrite::GetInstance()->CreateTextFormat(
            (LPWSTR) name.to_utf16().c_str(),
            nullptr,
            weight,
            style,
            stretch,
            size,
            L"",
            format.GetAddressOf()
        );

        if (FAILED(hr)) {
            BTK_LOG("FontImpl::lazy_eval : Failed to create text format");
        }
    }
    return format.Get();
}

// TextLayout
inline auto TextLayoutImpl::outline(float dpi) -> PainterPath {
    if (dpi != cached_path_dpi) {
        cached_path_dpi = dpi;
        cached_path.clear();
    }
    if (!cached_path.empty()) {
        return cached_path;
    }
    if (FAILED(DWriteGetTextLayoutOutline(lazy_eval(), dpi, &cached_path))) {
        // Failed, remove path
        cached_path.clear();
    }
    return cached_path;
}
inline auto TextLayoutImpl::lazy_eval() -> IDWriteTextLayout * {
    if (layout == nullptr) {
        // Create one
        assert(font);

        auto u16 = text.to_utf16();

        HRESULT hr = DWrite::GetInstance()->CreateTextLayout(
            (LPWSTR) u16.c_str(),
            u16.length(),
            font->lazy_eval(),
            max_width,
            max_height,
            layout.GetAddressOf()
        );

        if (FAILED(hr)) {
            BTK_LOG("TextLayoutImpl::lazy_eval : Failed to create text layout\n");
        }
    }
    return layout.Get();
}

// Font
COW_BASIC_IMPL(Font);

Font::Font() {
    priv = nullptr;
}
Font::Font(u8string_view name, float size) {
    priv = new FontImpl;
    priv->name = name;
    priv->size = size;

    priv->ref();
}
float Font::size() const {
    if (priv) {
        return priv->size;
    }
    return 0;
}
void  Font::set_size(float size) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    priv->size = size;
}
void  Font::set_bold(bool bold) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    if (bold) {
        priv->weight = DWRITE_FONT_WEIGHT_BOLD;
    }
    else {
        priv->weight = DWRITE_FONT_WEIGHT_BOLD;
    }
}
void  Font::set_italic(bool italic) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    if (italic) {
        priv->style = DWRITE_FONT_STYLE_ITALIC;
    }
    else {
        priv->style = DWRITE_FONT_STYLE_NORMAL;
    }
}
void  Font::set_family(u8string_view family) {
    begin_mut();
    priv->format.Reset();
    // Clear done

    priv->name = family;
}
void  Font::begin_mut() {
    COW_MUT(priv);
    priv->reset_manager();
}
bool  Font::empty() const {
    return priv == nullptr;
}
auto  Font::ListFamily() -> StringList {
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
    std::wstring tmp;
    
    ComPtr<IDWriteFontCollection> col;
    StringList ret;
    HRESULT hr = S_OK;
    BOOL exists = FALSE;
    BOOL has_local = FALSE;

    has_local = GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH);

    hr = DWrite::GetInstance()->GetSystemFontCollection(&col);
    if (FAILED(hr)) {
        return ret;
    }

    UINT32 n = col->GetFontFamilyCount();
    ret.reserve(n);

    for (UINT32 i = 0; i < n; i ++) {
        ComPtr<IDWriteFontFamily>    family;
        ComPtr<IDWriteLocalizedStrings> str;
        hr = col->GetFontFamily(i, &family);
        hr = family->GetFamilyNames(&str);

        // Find local string
        UINT32 index = 0;
        if (has_local) {
            hr = str->FindLocaleName(locale_name, &index, &exists);
        }
        if (SUCCEEDED(hr) || !exists) {
            // No name, try en
            hr = str->FindLocaleName(L"en-us", &index, &exists);
        }
        if (!exists) {
            // Oh no
            index = 0;
        }

        UINT32 len;
        hr = str->GetStringLength(index, &len);

        tmp.resize(len);

        hr = str->GetString(index, tmp.data(), len + 1);

        if (SUCCEEDED(hr)) {
            ret.emplace_back(u8string::from(tmp));
        }
    }
    return ret;
}
void Font::Init() {
    DWrite::Init();
}
void Font::Shutdown() {
    DWrite::Shutdown();
}
bool Font::native_handle(FontHandle what, void *out) const {
    if (what != FontHandle::IDWriteTextFormat) {
        return false;
    }
    if (!priv) {
        return false;
    }
    auto ft = priv->lazy_eval();
    if (!ft) {
        return false;
    }
    *static_cast<IDWriteTextFormat **>(out) = ft;
    return true;
}
void Font::bind_device_resource(void *key, PaintResource *res) const {
    if (priv) {
        priv->add_resource(key, res);
        return;
    }
    // No priv, try take it ownship and release it
    Ref<PaintResource> guard(res);
    return;
}
auto Font::query_device_resource(void *key) const -> PaintResource* {
    if (priv) {
        return priv->get_resource(key);
    }
    return nullptr;
}
// TextLayout
COW_BASIC_IMPL(TextLayout);

TextLayout::TextLayout() {
    priv = nullptr;
}
void TextLayout::set_text(u8string_view txt) {
    begin_mut();
    priv->text = txt;
}
void TextLayout::set_font(const Font &fnt) {
    begin_mut();
    priv->font.reset(fnt.priv);
}
void TextLayout::begin_mut() {
    COW_MUT(priv);
    // Clear state
    priv->layout.Reset(); //< Clear previous layout
    priv->cached_path_dpi = 0.0f;
    priv->cached_path.clear();
    priv->cached_size = {-1.0f, -1.0f};
    priv->reset_manager();
}
FSize TextLayout::size() const {
    if (priv) {
        if (priv->cached_size.is_valid()) {
            return priv->cached_size;
        }
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_TEXT_METRICS m;
            layout->GetMetrics(&m);
            // This size include white space
            priv->cached_size.w = m.widthIncludingTrailingWhitespace;
            priv->cached_size.h = m.height;
            return FSize(m.widthIncludingTrailingWhitespace, m.height);
        }
    }
    return FSize(-1, -1);
}
size_t TextLayout::line() const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_TEXT_METRICS m;
            layout->GetMetrics(&m);
            // This size include white space
            return m.lineCount;
        }
    }
    return 0;
}
Font  TextLayout::font() const {
    Font ft;
    if (priv) {
        ft.priv = COW_REFERENCE(priv->font.get());
    }
    return ft;
}
u8string_view TextLayout::text() const {
    if (priv) {
        return priv->text;
    }
    return { };
}
PainterPath   TextLayout::outline(float dpi) const {
    if (!priv) {
        return { };
    }
    return priv->outline(dpi);
}
bool TextLayout::rasterize(float dpi, std::vector<PixBuffer> *bitmaps, std::vector<Rect> *rect) const {
    if (!priv)  {
        return false;
    }
    auto lay = priv->lazy_eval();
    if (!lay) {
        return false;
    }
    return SUCCEEDED(DWriteGetTextLayoutBitmap(lay, dpi, bitmaps, rect));
}
bool TextLayout::hit_test(float x, float y, TextHitResult *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            BOOL inside;
            BOOL is_trailing_hit;
            DWRITE_HIT_TEST_METRICS m;
            if (FAILED(layout->HitTestPoint(x, y, &is_trailing_hit, &inside, &m))) {
                return false;
            }
            if (res) {
                res->text = m.textPosition;
                res->length = m.length;

                res->box.x = m.left;
                res->box.y = m.top;
                res->box.w = m.width;
                res->box.h = m.height;

                res->trailing = is_trailing_hit;
                res->inside = inside;
            }

            return true;
        }
    }
    return false;
}
bool TextLayout::hit_test_pos(size_t pos, bool trailing_hit, float *x, float *y, TextHitResult *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            DWRITE_HIT_TEST_METRICS m;
            if (FAILED(layout->HitTestTextPosition(pos, trailing_hit, x, y, &m))) {
                return false;
            }
            if (res) {
                res->text = m.textPosition;
                res->length = m.length;

                res->box.x = m.left;
                res->box.y = m.top;
                res->box.w = m.width;
                res->box.h = m.height;

                // res->trailing = is_trailing_hit;
                // res->inside = inside;
            }

            return true;
        }
    }
    return false;
}
bool TextLayout::hit_test_range(size_t text, size_t len, float org_x, float org_y, TextHitResults *res) const {
    if (priv) {
        auto layout = priv->lazy_eval();
        if (layout) {
            HRESULT hr;
            UINT32 count = 0;

            DWRITE_HIT_TEST_METRICS m;

            hr = layout->HitTestTextRange(text, len, org_x, org_y, &m, 0, &count);
            if (FAILED(hr) && hr != E_NOT_SUFFICIENT_BUFFER) {
                BTK_LOG("HitTestTextRange failed %d", hr);
                return false;
            }

            // Alloc the result array
            auto *hit_res = (DWRITE_HIT_TEST_METRICS*) _malloca(sizeof(DWRITE_HIT_TEST_METRICS) * count);
            hr = layout->HitTestTextRange(text, len, org_x, org_y, hit_res, count, &count);
            if (FAILED(hr)) {
                _freea(hit_res);
                return false;
            }

            // Copy the result
            if (res) {
                res->resize(count);
                for (size_t i = 0; i < count; i++) {
                    (*res)[i].text = hit_res[i].textPosition;
                    (*res)[i].length = hit_res[i].length;

                    (*res)[i].box.x = hit_res[i].left;
                    (*res)[i].box.y = hit_res[i].top;
                    (*res)[i].box.w = hit_res[i].width;
                    (*res)[i].box.h = hit_res[i].height;

                    // Unused properties
                    (*res)[i].trailing = true;
                    (*res)[i].inside = true;
                }
            }
            _freea(hit_res);
            return true;
        }
    }
    return false;
}
bool TextLayout::line_metrics(TextLineMetricsList *metrics) const {
    if (priv) {
        auto layout = priv->lazy_eval();

        DWRITE_LINE_METRICS m;
        HRESULT hr;
        UINT32 count = 0;

        hr = layout->GetLineMetrics(&m, 0, &count);
        if (FAILED(hr) && hr != E_NOT_SUFFICIENT_BUFFER) {
            BTK_LOG("GetLineMetrics failed %d", hr);
            return false;
        }

        // Alloc the result array
        auto *line_res = (DWRITE_LINE_METRICS*) _malloca(sizeof(DWRITE_LINE_METRICS) * count);
        hr = layout->GetLineMetrics(line_res, count, &count);
        if (FAILED(hr)) {
            _freea(line_res);
            return false;
        }

        // Copy result
        if (metrics) {
            metrics->resize(count);
            for (size_t i = 0; i < count; i++) { 
                (*metrics)[i].baseline = line_res[i].baseline;
                (*metrics)[i].height   = line_res[i].height;
            }
        }

        _freea(line_res);
        return true;
    }
    return false;
}
bool TextLayout::native_handle(TextLayoutHandle what, void *out) const {
    if (what != TextLayoutHandle::IDWriteTextLayout) {
        return false;
    }
    if (!priv) {
        return false;
    }
    auto lay = priv->lazy_eval();
    if (!lay) {
        return false;
    }
    *static_cast<IDWriteTextLayout **>(out) = lay;
    return true;
}
void TextLayout::bind_device_resource(void *key, PaintResource *res) const {
    if (priv) {
        priv->add_resource(key, res);
        return;
    }
    // No priv, try take it ownship and release it
    Ref<PaintResource> guard(res);
    return;
}
auto TextLayout::query_device_resource(void *key) const -> PaintResource* {
    if (priv) {
        return priv->get_resource(key);
    }
    return nullptr;
}

BTK_NS_END