#include "build.hpp"
#include "common/utils.hpp"
#include "common/mmap.hpp"

#include <Btk/detail/reference.hpp>
#include <Btk/painter.hpp>
#include <map>

extern "C" {
    #define STB_TRUETYPE_IMPLEMENTATION
    #define STBTT_STATIC

    #define STBTT_memcpy Btk_memcpy
    #define STBTT_memset Btk_memset
    #include "libs/stb_truetype.h"
}


BTK_PRIV_BEGIN

class GlyphMetrics {
    public:
        int width;
        int height;
        int advance_x;
        int bitmap_left;
        int bitmap_top;
};

/**
 * @brief File or Memory buffer
 * 
 */
class PhyFontSource : public Refable<PhyFontSource> {
    public:
        PhyFontSource(const void *buffer, size_t n);
        PhyFontSource(u8string_view path);
        ~PhyFontSource();
        
        const uint8_t *data() const {
            return static_cast<const uint8_t*>(buffer);
        }
        size_t size() const {
            return buffer_size;
        }
    private:
        u8string path; //< Path
        void *buffer = nullptr;
        size_t buffer_size = 0;
        bool   from_file = false;    
};

PhyFontSource::PhyFontSource(u8string_view path) {
    MapFile(path, &buffer, &buffer_size);
    from_file = true;
}
PhyFontSource::~PhyFontSource() {
    if (from_file) {
        UnmapFile(buffer, buffer_size);
        return;
    }
}

class PhyFontFace : public Refable<PhyFontFace> {
    public:
        PhyFontFace(Ref<PhyFontSource> source, int index);

        auto measure_text(float size, u8string_view text) -> Size;
        auto glyph_metrics(int glyph_index, float xscale, float yscale) -> GlyphMetrics;
    private:
        Ref<PhyFontSource> source;
        stbtt_fontinfo fontinfo;

        int raw_ascent;
        int raw_descent;
        int raw_line_gap;
};

PhyFontFace::PhyFontFace(Ref<PhyFontSource> source, int index) : source(source) {
    int offset = stbtt_GetFontOffsetForIndex(source->data(), index);
    if (offset < 0) {
        // Bad
    }

    if (!stbtt_InitFont(&fontinfo, source->data(), offset)) {
        // Bad

    }

    // Query info
    stbtt_GetFontVMetrics(&fontinfo, &raw_ascent, &raw_descent, &raw_line_gap);
}
auto PhyFontFace::measure_text(float size, u8string_view text) -> Size {
    float scale = stbtt_ScaleForMappingEmToPixels(&fontinfo, size);

    float ascent = raw_ascent * size;
    float decscent = raw_descent * size;
    float height = (raw_ascent - raw_descent) * size;

    int w = 0;
    int h = 0;

    for (uchar_t ch : text) {
        auto index = stbtt_FindGlyphIndex(&fontinfo, ch);
        auto glyph = glyph_metrics(index, scale, scale);

        int yoffset = ascent - glyph.bitmap_top;
        w += glyph.advance_x;

        h = max(h, glyph.height);
        h = max(h, yoffset + glyph.height);
    }
    return Size(w, h);
}
auto PhyFontFace::glyph_metrics(int index, float xscale, float yscale) -> GlyphMetrics {
    // Unscaled
    int   raw_advance;
    int   raw_lsb;

    // Scaled
    float advance;
    float lsb;

    int x0, y0, x1, y1; // Bitmap Box

    stbtt_GetGlyphHMetrics(&fontinfo, index, &raw_advance, &raw_lsb);
    stbtt_GetGlyphBitmapBox(&fontinfo, index, xscale, yscale, &x0, &y0, &x1, &y1);

    // Scale
    advance = raw_advance * xscale;
    lsb = raw_lsb * xscale;

    // Get Box of it
    GlyphMetrics m;
    m.width  = x1 - x0;
    m.height = y1 - y0;
    m.bitmap_left = x0;
    m.bitmap_top  = -y0;
    m.advance_x = advance;

    return m;
}

class FtRuntime final {
    public:
        // Map file to font data
        std::map<u8string, Ref<PhyFontSource>> source;
        // Map family name to face
        std::map<u8string, Ref<PhyFontFace>> faces;
};
class FtFont : public Refable<FtFont> {
    public:
        u8string family;
        float    size;

        Ref<PhyFontFace> face;
};
class FtTextLayout : public Refable<FtTextLayout> {
    public:
        Ref<FtFont> font;
        u8string    text;

        float       width_limit = std::numeric_limits<float>::max();
        float       height_limit = std::numeric_limits<float>::max();

};

BTK_PRIV_END


BTK_NS_BEGIN

class FontImpl final       : public FtFont { };
class TextLayoutImpl final : public FtTextLayout { };

BTK_NS_END