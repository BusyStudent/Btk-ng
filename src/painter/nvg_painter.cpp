#include "build.hpp"
#include "common.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(u, v) Btk_malloc(u)
#define STBTT_free(u, v)   Btk_free(u)
#define STBTT_STATIC
#include "libs/stb_truetype.h"

#include <unordered_map>

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

class PhyBlob : public Refable<PhyBlob> {
    public:
        pointer_t data;
        size_t    size;
};

class PhyFont : public Refable<PhyFont> {
    public:
        bool has_uchar(uchar_t ch) {
            return stbtt_FindGlyphIndex(&info, ch) != 0;
        }
        bool initialize(uint32_t index) {
            int off = stbtt_GetFontOffsetForIndex(
                static_cast<uint8_t*>(blob->data), index
            );
            if (off < 0) {
                return false;
            }
            return stbtt_InitFont(&info, static_cast<uint8_t*>(blob->data), off);
        }
        int  num_faces() const {
            return stbtt_GetNumberOfFonts(static_cast<uint8_t*>(blob->data));
        }
        stbtt_fontinfo info;
        Ref<PhyBlob>   blob;
};
class FontCollection : public Refable<FontCollection> {
    public:
        std::unordered_map<u8string, Ref<PhyFont>> phy_fonts;
};

class FontAtlas {
    public:
        Ref<FontCollection> sys_col;
};
class FontContext {

};



BTK_NS_END2()