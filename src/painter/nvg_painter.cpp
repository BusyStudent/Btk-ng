// nanovg interface
//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// Copyright (c) 2022 BusyStudent 
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "build.hpp"
#include "common/utils.hpp"

// Import library
#include "libs/opengl_macro.hpp"

// Import freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <Btk/painter.hpp>
#include <Btk/context.hpp>
#include <unordered_map>
#include <filesystem>
#include <variant>
#include <set>

// Forward declaration for nanovg
#define FONS_GLYPH_BITMAP_REQUIRED 1
#define FONS_GLYPH_BITMAP_OPTIONAL 0

#define FONS_ZERO_TOPLEFT          0
#define FONS_INVALID              -1

#define FONS_DROP(fn) \
    template<typename ...Args> \
    inline int fn(Args &&...args) { \
        BTK_ASSERT(false && "Undefined fn : " #fn);\
        return FONS_INVALID;\
    }
#define NVG_UNSUPPORT(fn) BTK_LOG("Nanovg backend unsupport " #fn "\n")

namespace {
    using namespace BTK_NAMESPACE;

    struct FONScontext {
        // hidden...
    };
    struct FONSparams {
        int width, height;
        unsigned char flags;
        void* userPtr;
        int (*renderCreate)(void* uptr, int width, int height);
        int (*renderResize)(void* uptr, int width, int height);
        void (*renderUpdate)(void* uptr, int* rect, const unsigned char* data);
        void (*renderDraw)(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
        void (*renderDelete)(void* uptr);
    };
    struct FONStextIter {
        float x, y, nextx, nexty, scale, spacing;
        unsigned int codepoint;
        
        short isize, iblur;

        Font *font;
        int64_t prevGlyphIndex; //< I think int64 is better than int
        const char* str;
        const char* next;
        const char* end;
        int bitmapOption;
    };
    struct FONSquad {
        float x0, y0, s0, t0;
        float x1, y1, s1, t1;
    };

    class GLFunctions {
        public:
            #define BTK_GL_PROCESS(pfn, name) \
                pfn name = nullptr;
            BTK_OPENGLES3_FUNCTIONS
            #undef  BTK_GL_PROCESS

            #define BTK_GL_PROCESS(pfn, name) \
                name = reinterpret_cast<pfn>(ctxt->get_proc_address(#name)); \
                if (!name) { \
                    bad_fn(#name); \
                }
            void load (GLContext *ctxt) {
                BTK_OPENGLES3_FUNCTIONS
            }
            #undef  BTK_GL_PROCESS


            // Got nullptr on load
            void bad_fn(const char *name) {
                BTK_THROW(std::runtime_error(u8string::format(
                    "Failed to load OpenGL function: %s", name
                )));
            }
    };

    class GLFunctionsProxy {
        public:
            #define BTK_GL_PROCESS(pfn, name) \
                template <typename ...Args> \
                auto name(Args &&...args) { \
                    return funcs->name(std::forward<Args>(args)...);\
                }
            BTK_OPENGLES3_FUNCTIONS
            #undef  BTK_GL_PROCESS

            GLFunctions *funcs;
    };

    FONScontext *fonsCreateInternal(FONSparams *param);
    void         fonsDeleteInternal(FONScontext *ctx);

    uint8_t     *fonsGetTextureData(FONScontext *s, int* width, int* height);
    bool         fonsValidateTexture(FONScontext *s, int *dirty);

    void         fonsExpandAtlas(FONScontext *s, int width, int height);
    void         fonsResetAtlas(FONScontext *s, int width, int height);

    void         fonsSetSpacing(FONScontext *s, float spacing);
    void         fonsSetSize(FONScontext *s, float size);
    void         fonsSetFont(FONScontext *s, int font);
    void         fonsSetAlign(FONScontext *s, int align);

    float        fonsTextBounds(FONScontext *s, float x, float y, const char* str, const char* end, float* bounds);
    void         fonsLineBounds(FONScontext *s, float y, float* miny, float* maxy);
    void         fonsVertMetrics(FONScontext *s, float* ascender, float* descender, float* lineh);

    bool         fonsTextIterInit(FONScontext *s, FONStextIter* iter, float x, float y, const char* str, const char* end, int bitmapOption);
    bool         fonsTextIterNext(FONScontext *s, FONStextIter* iter, FONSquad* quad);

    // Unsupport blur temp
    FONS_DROP   (fonsSetBlur);
    FONS_DROP   (fonsAddFont);
    FONS_DROP   (fonsAddFontMem);
    FONS_DROP   (fonsAddFallbackFont);
    FONS_DROP   (fonsAddFallbackFontId);
    FONS_DROP   (fonsResetFallbackFont);
    FONS_DROP   (fonsGetFontByName);

    // Utils
    inline int64_t FixedFloor(int64_t x) {
        return ((x & -64) / 64);
    }
    inline int64_t FixedCeil(int64_t x) {
        return (((x + 63) & -64) / 64);
    }
}

// Import nanovg
#define  NVG_NO_STB
#define  NANOVG_GL_IMPLEMENTATION
#define  NANOVG_GLES3
#include "libs/nanovg.hpp"
#include "libs/nanovg_gl.hpp"
#include "libs/nanovg.cpp"



BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

// Forward declaration
class FontContext;
class PhyFont;
class PhyBlob;


class UnCopyable {
    public:
        UnCopyable() = default;
        UnCopyable(const UnCopyable &) = delete;
};
class GlyphMetrics {
    public:
        int width;
        int height;
        int bitmap_left;
        int bitmap_top;
        int advance_x;
        int advance_y;
};
class FontMetrics {
    public:
        int ascender;
        int descender;
        int height;
        int max_advance;
        int underline_position;
        int underline_thickness;
};
class FontGlyph {
    public:
        GlyphMetrics m; //< Glyph Metrics
        PhyFont  *font; //< Font
        short x, y; //< In bitmap position
        short size; //< Size of font
        uint32_t idx; //< Glyph index
};

class PhyBlob : public WeakRefable<PhyBlob>, public UnCopyable {
    public:
        PhyBlob() = default;
        PhyBlob(u8string_view fname);
        ~PhyBlob();

        bool valid() const noexcept {
            return data != nullptr && size > 0;
        }
        bool compare(const PhyBlob &b) const noexcept {
            return Btk_memcmp(this, &b, sizeof(PhyBlob)) == 0;
        }

        void    (*free)(PhyBlob *) = nullptr;
        pointer_t data = nullptr;
        size_t    size = 0;
};

class PhyFont : public WeakRefable<PhyFont>, public UnCopyable {
    public:
        PhyFont() = default;
        PhyFont(const Ref<PhyBlob> &blob);
        ~PhyFont();

        bool initailize(int index = 0);
        auto metrics() -> FontMetrics;

        // Set size
        void set_size(float xdpi, float ydpi, float size);

        // Query
        int  num_of_faces() const noexcept { return face->num_faces; }
        int  face_index() const noexcept { return face->face_index & 0xFFFF; }

        auto glyph_metrics(uint32_t idx) -> GlyphMetrics;
        auto glyph_index(uchar_t codepoint) -> uint32_t;
        auto glyph_render(uint32_t idx, void *buf, int pitch, int pen_x, int pen_y) -> void;


        Ref<PhyBlob>   blob = {};
        FT_Face        face = {};
        FontContext   *ctxt = {};
        int             id  = {};
};

// For cache Glyph
class AtlasNode {
    public:
        short x = 0;
        short y = 0;
        short width = 0;
};
class FontAtlas : public FONScontext, public UnCopyable {
    public:
        FontAtlas() : FontAtlas(521, 512) {}
        FontAtlas(int w, int h);
        ~FontAtlas();

        void add_dirty(int x, int y, int w, int h);
        bool validate(int *dirty);
        uint8_t *data(); 

        // Measure
        float text_bounds(float x, float y, const char* str, const char* end, float* bounds);
        void  line_bounds(float y, float* miny, float* maxy);
        void  metrics(float* ascender, float* descender, float* lineh);

        FSize measure_text(u8string_view txt);

        void  transform_back(float w, float h,float *x, float *y);

        // Atlas interface
        void expend(int w, int h);
        void reset(int w, int h);

        void atlas_init(int w, int h);
        void atlas_cleanup();
        void atlas_expend(int w, int h);
        void atlas_reset(int w, int h);
        void atlas_remove_node(int index);
        bool atlas_insert_node(int index,int x,int width,int height);
        bool atlas_add_skyline(int index,int x,int y,int w,int h);
        int  atlas_rect_fits(int i,int w,int h);
        bool atlas_add_rect(int x,int y,int *w,int *h);

        // Glyph
        FontGlyph *get_glyph(PhyFont *font, uchar_t codepoint, bool need_bitmap);


        std::vector<uint8_t> bitmap; //< Gray scale bitmap
        int   dirty[4]; //< Left | Top => Right | Bottom

        // Packer field
        int   width;
        int   height;
        int   nnodes;
        int   cnodes;
        AtlasNode* nodes;

        // State
        
        float size    = 0;
        float spacing = 0;

        float xdpi    = 72;
        float ydpi    = 72;

        int   font    = FONS_INVALID;

        Alignment align = Alignment::Left | Alignment::Top;

        // Font used in this atlas
        std::vector<WeakRef<PhyFont>> fonts;

        // Cached glyph [codepoint => glyph]
        std::unordered_multimap<uchar_t, FontGlyph> glyphs;
};

class FtInitilizer : public UnCopyable {
    public:
        FtInitilizer() {
            FT_Init_FreeType(&ft);
        }
        ~FtInitilizer() {
            FT_Done_FreeType(ft);
        }
        FT_Library ft = {};
};

// Main runtime
class FontContext : public FtInitilizer {
    public:
        Ref<PhyBlob> openfile(u8string_view v) {
            auto iter = blobs.find(u8string(v));
            if (iter != blobs.end()) {
                auto b = iter->second;
                if (b.expired()) {
                    // Is already destroyed
                    blobs.erase(iter);
                }
                else {
                    return b.lock();
                }
            }

            auto b = PhyBlob::New(v);
            if (b->valid()) {
                // Insert into cache
                blobs.insert({u8string(v), b});
                return b;
            }
            return {};
        }
        Ref<PhyFont> newfont(const Ref<PhyBlob> &b, int idx) {
            // Flush cache
            for (auto iter = fonts.begin(); iter != fonts.end();) {
                if (iter->second.expired()) {
                    iter = fonts.erase(iter);
                    continue;
                }
                // Hit
                auto f = iter->second.lock();
                if (f->blob == b && f->face_index() == idx) {
                    return f;
                }
            }

            auto f = PhyFont::New(b);
            f->ctxt = this;

            if (!f->initailize(idx)) {
                return {};
            }

            // Make a index for font
            int id = 0;
            do {
                id = std::rand();
            }
            while(id < 0 || fonts.find(id) != fonts.end());

            f->id = id;

            return f;
        }
        Ref<PhyFont> query_font(int id) {
            auto iter = fonts.find(id);
            if (iter == fonts.end()) {
                // TODO :
            }
            return iter->second.lock();
        }
        // For storeing fonts
        std::unordered_map<u8string, WeakRef<PhyBlob>> blobs;
        std::unordered_map<int     , WeakRef<PhyFont>> fonts;

        // For measure text without painter
        FontAtlas                                      atlas;
};
class PainterRuntime : public FontContext {
    public:
        std::set<GLContext *> shared_ctxt;
};

// Basic declation end
static PainterRuntime *pt_runtime  = nullptr;
static int             pt_refcount = 0;


// Some helpers
static StringList   enum_system_fonts() {

#if defined(_WIN32)
    std::filesystem::path path(L"C:\\Windows\\Fonts");

    StringList list;

    for (auto &p : std::filesystem::directory_iterator(path)) {
        if (p.is_regular_file()) {
            auto name = p.path().filename().u8string();
            if (name.find(".ttf") != std::string::npos) {
                list.push_back(u8string(name));
            }
        }
    }

    return list;
#elif defined(__gnu_linux__)
    return StringList();
#endif

}

// Font Implementation
PhyFont::~PhyFont() {
    if (face) {
        FT_Done_Face(face);
    }
}
bool PhyFont::initailize(int index) {
    if (face) {
        // Clean up previous face
        FT_Done_Face(face);
    }
    if (!blob || !ctxt) {
        // No blob or context
        return false;
    }

    FT_Error err = FT_New_Memory_Face(
        ctxt->ft, 
        static_cast<FT_Byte*>(blob->data), 
        blob->size, 
        index, 
        &face
    );

    return err == FT_Err_Ok;
}
auto PhyFont::metrics() -> FontMetrics {
    FontMetrics metrics;
    if(FT_IS_SCALABLE(face)){
        FT_Fixed scale = face->size->metrics.y_scale;
        metrics.ascender   = FixedCeil(FT_MulFix(face->ascender, scale));
        metrics.descender  = FixedCeil(FT_MulFix(face->descender, scale));
        metrics.height   = FixedCeil(FT_MulFix(face->ascender - face->descender, scale));
        metrics.underline_position = FixedFloor(FT_MulFix(face->underline_position, scale));
        metrics.underline_thickness = FixedFloor(FT_MulFix(face->underline_thickness, scale));
        metrics.max_advance = face->size->metrics.max_advance >> 6;
    }
    else{
        metrics.ascender    = face->size->metrics.ascender >> 6;
        metrics.descender   = face->size->metrics.descender >> 6;
        metrics.height      = face->size->metrics.height >> 6;
        metrics.max_advance = face->size->metrics.max_advance >> 6;
        metrics.underline_position = face->underline_position >> 6;
        metrics.underline_thickness = face->underline_thickness >> 6;
    }
    return metrics;
}
void PhyFont::set_size(float size, float xdpi, float ydpi) {
    FT_Set_Char_Size(
        face, 
        size * 64,
        size * 64,
        xdpi,
        ydpi
    );
}
auto PhyFont::glyph_index(uchar_t codepoint) -> uint32_t {
    return FT_Get_Char_Index(face, codepoint);
}
auto PhyFont::glyph_metrics(uint32_t idx) -> GlyphMetrics {
    FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);
    FT_GlyphSlot slot = face->glyph;

    GlyphMetrics m;
    m.width = slot->bitmap.width;
    m.height = slot->bitmap.rows;
    m.bitmap_left = slot->bitmap_left;
    m.bitmap_top  = slot->bitmap_top;
    m.advance_x   = slot->advance.x >> 6;

    return m;
}
auto PhyFont::glyph_render(uint32_t idx, void *buf, int pitch, int pen_x, int pen_y) -> void {
    FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
    FT_GlyphSlot slot = face->glyph;

    uint8_t *pixels = static_cast<uint8_t*>(buf);
    for (int y = 0;y < slot->bitmap.rows; y++) {
        for (int x = 0;x < slot->bitmap.width; x++) {
            pixels[
                (pen_y + y) * pitch +
                (pen_x + x)
            ] = slot->bitmap.buffer[y * slot->bitmap.width + x];
        }
    }
}

// Blob implementation
PhyBlob::PhyBlob(u8string_view file) {
    // Read file to memory directly, We should use mmap instead
    FILE *fp;
#if defined(_WIN32)
    fp = _wfopen(reinterpret_cast<const wchar_t*>(file.to_utf16().c_str()), L"rb");
#else
    fp = fopen(file.data(), "rb");
#endif

    if (!fp) {
        return;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = Btk_malloc(size);
    fread(data, size, 1, fp);
    fclose(fp);

    free = [](PhyBlob *blob) {
        Btk_free(blob->data);
    };
}
PhyBlob::~PhyBlob() {
    if (free) {
        free(this);
    }
}

// FontAtlas
FontAtlas::FontAtlas(int w, int h) : bitmap(w * h), dirty{w, h, 0, 0}, width(w), height(h) {
    atlas_init(w, h);
}
FontAtlas::~FontAtlas() {
    atlas_cleanup();
}
void FontAtlas::add_dirty(int x, int y, int w, int h) {
    dirty[0] = min(dirty[0], x);
    dirty[1] = min(dirty[1], y);
    dirty[2] = max(dirty[2], x + w);
    dirty[3] = max(dirty[3], y + h);
}
bool FontAtlas::validate(int *d) {
    if (dirty[0] < dirty[2] && dirty[1] < dirty[3]) {
        // Has dirty
        for (int i = 0;i < 4; i++) {
            d[i] = dirty[i];
        }

        dirty[0] = width;
        dirty[1] = height;
        dirty[2] = 0;
        dirty[3] = 0;
        return true;
    }
    return false;
}

void FontAtlas::expend(int w, int h) {
    auto prev_w = width;
    auto prev_h = height;
    w = max(w, width);
    h = max(h, height);

    if(w == width && h == height){
        return;
    }
    
    std::vector<uint8_t> new_map(w * h);
    //Copy old map to new map
    for(int y = 0;y < width; y++) {
        for(int x = 0;x < height; x++) {
            new_map[y * w + x] = bitmap[y * width + x];
        }
    }
    //Increase atlas size
    atlas_expend(w, h);

    //Mark as dirty
    short maxy = 0;
    for(int i = 0;i < nnodes;i++){
        maxy = max(maxy, nodes[i].y);
    }

    dirty[0] = 0;
    dirty[1] = 0;
    dirty[2] = prev_w;
    dirty[3] = maxy;

    bitmap = std::move(new_map);
}
void FontAtlas::reset(int w, int h) {
    bitmap.resize(w * h);
    atlas_reset(w, h);
    dirty[0] = w;
    dirty[1] = h;
    dirty[2] = 0;
    dirty[3] = 0;

    glyphs.clear();
}

FontGlyph *FontAtlas::get_glyph(PhyFont *ft, uchar_t codepoint, bool need_bitmap) {
    auto iter = glyphs.find(codepoint);
    auto num  = glyphs.count(codepoint);

    FontGlyph *glyph = nullptr;

    if (iter != glyphs.end()) {
        // Hit
        while (num) {
            -- num;
            auto &gl = iter->second;
            if (gl.font != ft || gl.size == gl.size) {
                glyph = &gl;
                break;
            }
        }
    }
    if (!glyph) {
        // Make a new one
        ft->set_size(size, xdpi, ydpi);
        auto idx = ft->glyph_index(codepoint);
        auto metrics = ft->glyph_metrics(idx);

        FontGlyph g;
        g.font = ft;
        g.idx = idx;
        g.m = metrics;
        g.size = size;
        g.x = -1;
        g.y = -1;

        auto iter = glyphs.emplace(std::make_pair(codepoint, g));
        glyph = &iter->second;
    }


    if (need_bitmap && glyph->x < 0 & glyph->y < 0) {
        // Need raster
        ft->set_size(size, xdpi, ydpi);
        
        // Alloc space
        int x, y;
        if (!atlas_add_rect(glyph->m.width, glyph->m.height, &x, &y)) {
            // No encough space
            return nullptr;
        }

        glyph->x = x;
        glyph->y = y;

        ft->glyph_render(glyph->idx, bitmap.data(), width, x, y);

        add_dirty(x, y, glyph->m.width, glyph->m.height);
    }
    return glyph;
}
FSize FontAtlas::measure_text(u8string_view txt) {
    auto ft = pt_runtime->query_font(font);

    // Query Global metrics
    ft->set_size(size, xdpi, ydpi);
    auto metrics = ft->metrics();

    float w = 0;
    float h = 0;

    uint32_t prev = 0;
    bool first = true;

    for (uchar_t codepoint : txt) {
        if (!first) {
            w += spacing;
        }
        else {
            first = false;
        }
        auto g = get_glyph(ft.get(), codepoint, FONS_GLYPH_BITMAP_OPTIONAL);
        int yoffset = metrics.ascender - g->m.bitmap_top;

        h = max<float>(h, g->m.height);
        h = max<float>(h, g->m.height + yoffset);
        w += g->m.advance_x;
    }
    return FSize(w, h);
}
float FontAtlas::text_bounds(float x, float y, const char* str, const char* end, float* bounds) {
    if (!end) {
        end = str + strlen(str);
    }
    auto [w, h] = measure_text(u8string_view(str, end - str));

    // Back to Left | Top
    transform_back(w, h, &x, &y);

    if (bounds) {
        bounds[0] = x;
        bounds[1] = y;
        bounds[2] = x + w;
        bounds[3] = y + h;
    }

    return w;
}
void FontAtlas::line_bounds(float y, float *miny, float *maxy){
    auto ft = pt_runtime->query_font(font);
    
    // Query Global metrics
    ft->set_size(size, xdpi, ydpi);
    auto metrics = ft->metrics();

    if ((align & Alignment::Top) == Alignment::Top) {
        // Nothing
    }
    else if ((align & Alignment::Middle) == Alignment::Middle) {
        y -= metrics.height / 2;
    }
    else if ((align & Alignment::Baseline) == Alignment::Middle) {
        y -= metrics.ascender;
    }
    else if ((align & Alignment::Bottom) == Alignment::Bottom) {
        y -= metrics.height;
    }

    if (miny) {
        *miny = y;
    }
    if (maxy) {
        *maxy = y + metrics.height;
    }
}
void FontAtlas::transform_back(float w, float h,float *x, float *y) {
    if ((align & Alignment::Right) == Alignment::Right) {
        *x -= w;
    }
    if ((align & Alignment::Bottom) == Alignment::Bottom) {
        *y -= h;
    }
    if ((align & Alignment::Center) == Alignment::Center) {
        *x -= w / 2;
    }
    if ((align & Alignment::Middle) == Alignment::Middle) {
        *y -= h / 2;
    }
    if ((align & Alignment::Baseline) == Alignment::Baseline) {
        // TODO 
        *y -= h / 2;
    }
}

// Atlas packer
void FontAtlas::atlas_init(int w, int h) {
    // Allocate memory for the font stash.
    int n = 255;
    
    width = w;
    height = h;

    // Allocate space for skyline nodes
    nodes = (AtlasNode*)Btk_malloc(sizeof(AtlasNode) * n);
    Btk_memset(nodes, 0, sizeof(AtlasNode) * n);
    nnodes = 0;
    cnodes = n;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
void FontAtlas::atlas_cleanup() {
    Btk_free(nodes);
}
bool FontAtlas::atlas_insert_node(int idx, int x, int y, int w) {
    int i;
    // Insert node
    if (nnodes+1 > cnodes) {
        cnodes = cnodes == 0 ? 8 : cnodes * 2;
        nodes = (AtlasNode*)Btk_realloc(nodes, sizeof(AtlasNode) * cnodes);
        if (nodes == nullptr)
            return false;
    }
    for (i = nnodes; i > idx; i--)
        nodes[i] = nodes[i-1];
    nodes[idx].x = (short)x;
    nodes[idx].y = (short)y;
    nodes[idx].width = (short)w;
    nnodes++;
    return true;
}
void FontAtlas::atlas_remove_node(int idx) {
    int i;
    if (nnodes == 0) return;
    for (i = idx; i < nnodes-1; i++)
        nodes[i] = nodes[i+1];
    nnodes--;
}
void FontAtlas::atlas_expend(int w,int h) {
    // Insert node for empty space
    if (w > width)
        atlas_insert_node(nnodes, width, 0, w - width);
    width = w;
    height = h;
}
void FontAtlas::atlas_reset(int w,int h) {
    width = w;
    height = h;
    nnodes = 0;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
bool FontAtlas::atlas_add_skyline(int idx, int x, int y, int w, int h) {
    int i;

    // Insert new node
    if (atlas_insert_node(idx, x, y+h, w) == 0)
        return false;

    // Delete skyline segments that fall under the shadow of the new segment.
    for (i = idx+1; i < nnodes; i++) {
        if (nodes[i].x < nodes[i-1].x + nodes[i-1].width) {
            int shrink = nodes[i-1].x + nodes[i-1].width - nodes[i].x;
            nodes[i].x += (short)shrink;
            nodes[i].width -= (short)shrink;
            if (nodes[i].width <= 0) {
                atlas_remove_node(i);
                i--;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    // Merge same height skyline segments that are next to each other.
    for (i = 0; i < nnodes-1; i++) {
        if (nodes[i].y == nodes[i+1].y) {
            nodes[i].width += nodes[i+1].width;
            atlas_remove_node(i+1);
            i--;
        }
    }

    return true;
}
int  FontAtlas::atlas_rect_fits(int i, int w, int h){
    // Checks if there is enough space at the location of skyline span 'i',
    // and return the max height of all skyline spans under that at that location,
    // (think tetris block being dropped at that position). Or -1 if no space found.
    int x = nodes[i].x;
    int y = nodes[i].y;
    int spaceLeft;
    if (x + w > width)
        return -1;
    spaceLeft = w;
    while (spaceLeft > 0) {
        if (i == nnodes) return -1;
        y = std::max<int>(y, nodes[i].y);
        if (y + h > height) return -1;
        spaceLeft -= nodes[i].width;
        ++i;
    }
    return y;
}
bool FontAtlas::atlas_add_rect(int rw, int rh, int* rx, int* ry){

    int besth = height, bestw = width, besti = -1;
    int bestx = -1, besty = -1, i;

    // Bottom left fit heuristic.
    for (i = 0; i < nnodes; i++) {
        int y = atlas_rect_fits(i, rw, rh);
        if (y != -1) {
            if (y + rh < besth || (y + rh == besth && nodes[i].width < bestw)) {
                besti = i;
                bestw = nodes[i].width;
                besth = y + rh;
                bestx = nodes[i].x;
                besty = y;
            }
        }
    }

    if (besti == -1)
        return false;

    // Perform the actual packing.
    if (atlas_add_skyline(besti, bestx, besty, rw, rh) == 0)
        return false;

    *rx = bestx;
    *ry = besty;

    return 1;
}


BTK_NS_END2()


BTK_NS_BEGIN

class PainterPathNode {
    public:
        enum {
            ArcTo,
            MoveTo,
            LineTo,
            QuadTo,
            BezierTo,
        } type;

        float x, y;
        float cx, cy, cx1, cy1, cx2, cy2;
};

class PainterPathImpl {
    public:
        // std::vector<PainterPathNode> nodes;
        // This way save more memory
        std::vector<float> data;

        // Opcode : Datalen
        // MoveTo   x , y    3 float
        // LineTo   x , y    3 float
        // QuadTo   xy, xy   5 float
        // ClosePath  [Empty] 1 float
};

class BrushImpl   : public Refable<BrushImpl>, public Trackable {
    public:
        std::variant<
            LinearGradient, 
            RadialGradient, 
            PixBuffer, 
            GLColor
        > data = Color::Black;
        CoordinateMode mode = CoordinateMode::Relative;
        BrushType      type = BrushType::Solid;
};
class FontImpl    : public Refable<FontImpl> {
    public:
        u8string     name; //< Logical name, System will match with name
        Ref<PhyFont> font;
        float        size = 0.0f;
        bool         bold = false;
        bool         italic = false;
};
class PenImpl     : public Refable<PenImpl> {
    public:
        LineJoin     join = LineJoin::Miter;
        LineCap      cap  = LineCap::Flat;
        float        miter_limit = 10.0f;
};

class TextureImpl : public Trackable {
    public:
        PainterImpl *painter;
        GLuint       image;
        int          id;
};

class PainterImpl : public GLFunctions, public Trackable {
    public:
        PainterImpl(AbstractWindow *win);
        ~PainterImpl();

        void clear();

        AbstractWindow *win = nullptr;
        GLColor         color = Color::Black;
        GLContext      *ctxt  = nullptr;
        NVGcontext     *nvg_ctxt = nullptr;

        GLint       max_texture_size[2];

        FontAtlas   atlas;
};

inline PainterImpl::PainterImpl(AbstractWindow *win) {
    ctxt = static_cast<GLContext*>(win->gc_create("opengl"));
    if (ctxt == nullptr) {
        BTK_THROW(std::runtime_error("Failed to create OpenGL context"));
    }
    if (!ctxt->initialize(GLFormat())) {
        BTK_THROW(std::runtime_error("Failed to initialize OpenGL context"));
    }

    // Load OpenGL functions
    load(ctxt);

    // Query info
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, max_texture_size);

    // Init state
    nvg_ctxt = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES, this);
}
inline PainterImpl::~PainterImpl() {
    nvgDeleteGLES3(nvg_ctxt);
    delete ctxt;
}
inline void PainterImpl::clear() {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}



void Painter::Init() {
    BTK_ONCE(BTK_LOG("NanoVG : This painter is experimental.\n"))

    if (pt_refcount == 0) {
        pt_runtime = new PainterRuntime();
    }
    pt_refcount++;
}
void Painter::Shutdown() {
    if (--pt_refcount == 0) {
        delete pt_runtime;
        pt_runtime = nullptr;
    }
}
void Painter::begin() {
    priv->ctxt->begin();

    auto [fb_w, fb_h] = priv->ctxt->get_drawable_size();
    auto [w, h] = priv->win->size();

    priv->glViewport(0, 0, fb_w, fb_h);

    nvgBeginFrame(priv->nvg_ctxt, w, h, fb_w / w);
}
void Painter::end() {
    // NVG Cleanup
    nvgEndFrame(priv->nvg_ctxt);
    // GL Cleanup
    priv->ctxt->end();
    priv->ctxt->swap_buffers();
}
void Painter::clear() {
    priv->clear();
}

void Painter::set_antialias(bool v) {
    nvgShapeAntiAlias(priv->nvg_ctxt, v);
}
void Painter::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    priv->color = Color(r, g, b, a);
    nvgStrokeColor(priv->nvg_ctxt, nvgRGBA(r, g, b, a));
}
void Painter::set_colorf(float r, float g, float b, float a) {
    priv->color = GLColor(r, g, b, a);
    nvgStrokeColor(priv->nvg_ctxt, nvgRGBAf(r, g, b, a));
}
void Painter::set_font(const Font &f) {
    int id = FONS_INVALID;
    if (!f.empty()) {
        id = f.priv->font->id;
    }
    nvgFontFaceId(priv->nvg_ctxt, id);
}
void Painter::set_brush(const Brush &b) {

}
void Painter::set_stroke_width(float w) {
    nvgStrokeWidth(priv->nvg_ctxt, w);
}

// Draw / Fill

void Painter::draw_rect(float x, float y, float w, float h) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgRect(priv->nvg_ctxt, x, y, w, h);
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_circle(float x, float y, float r) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgCircle(priv->nvg_ctxt, x, y, r);
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_ellipse(float x, float y, float rx, float ry) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgEllipse(priv->nvg_ctxt, x, y, rx, ry);
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_rounded_rect(float x, float y, float w, float h, float r) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgRoundedRect(priv->nvg_ctxt, x, y, w, h, r);
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_line(float x1, float y1, float x2, float y2) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgMoveTo(priv->nvg_ctxt, x1, y1);
    nvgLineTo(priv->nvg_ctxt, x2, y2);
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_lines(const FPoint *fp, size_t n) {
    if (n < 1) {
        return;
    }
    nvgBeginPath(priv->nvg_ctxt);
    nvgMoveTo(priv->nvg_ctxt, fp[0].x, fp[0].y);
    for (size_t i = 1; i < n; i++) {
        nvgLineTo(priv->nvg_ctxt ,fp[i].x, fp[i].y);
    }
    nvgStroke(priv->nvg_ctxt);
}
void Painter::draw_text(u8string_view txt, float x, float y) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgText(priv->nvg_ctxt, x, y, txt.data(), txt.data() + txt.size());
    nvgStroke(priv->nvg_ctxt);
}

void Painter::fill_rect(float x, float y, float w, float h) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgRect(priv->nvg_ctxt, x, y, w, h);
    nvgFill(priv->nvg_ctxt);
}
void Painter::fill_circle(float x, float y, float r) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgCircle(priv->nvg_ctxt, x, y, r);
    nvgFill(priv->nvg_ctxt);
}
void Painter::fill_ellipse(float x, float y, float rx, float ry) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgEllipse(priv->nvg_ctxt, x, y, rx, ry);
    nvgFill(priv->nvg_ctxt);
}
void Painter::fill_rounded_rect(float x, float y, float w, float h, float r) {
    nvgBeginPath(priv->nvg_ctxt);
    nvgRoundedRect(priv->nvg_ctxt, x, y, w, h, r);
    nvgFill(priv->nvg_ctxt);
}

void Painter::push_scissor(float x, float y, float w, float h) {
    nvgSave(priv->nvg_ctxt);
    nvgIntersectScissor(priv->nvg_ctxt, x, y, w, h);
}
void Painter::pop_scissor() {
    nvgRestore(priv->nvg_ctxt);
}

Painter Painter::FromWindow(AbstractWindow *win) {
    return Painter(new PainterImpl(win));
}

// Brush
COW_IMPL(Brush);
Brush::Brush() {
    priv = nullptr;
}

void Brush::set_color(const GLColor &c) {
    begin_mut();
    priv->type = BrushType::Solid;
    priv->data = c;
}
void Brush::set_image(const PixBuffer &buf) {
    begin_mut();
    priv->type = BrushType::Texture;
    priv->data = buf;
}
void Brush::set_gradient(const LinearGradient &gr) {
    begin_mut();
    priv->type = BrushType::LinearGradient;
    priv->data = gr;
}
void Brush::set_gradient(const RadialGradient &gr) {
    begin_mut();
    priv->type = BrushType::RadialGradient;
    priv->data = gr;
}

GLColor Brush::color() const {
    if (priv && priv->type == BrushType::Solid) {
        return std::get<GLColor>(priv->data);
    }
    return Color::Black;
}
BrushType Brush::type() const {
    if (!priv) {
        return BrushType::Solid;
    }
    return priv->type;
}

// Font
COW_IMPL(Font);
Font::Font(u8string_view name, float size) {
    priv = new FontImpl;

    priv->name = name;
    priv->size = size;

    priv->ref();
}
Font::Font() {
    priv = nullptr;
}
void  Font::set_size(float size) {
    begin_mut();
    priv->size = size;
}
void  Font::set_bold(bool b) {
    begin_mut();
    priv->bold = b;
}
void  Font::set_italic(bool i) {
    begin_mut();
    priv->italic = i;
}
float Font::size() const {
    if (priv) {
        return priv->size;
    }
    return 0;
}

// Pen
COW_IMPL(Pen);
Pen::Pen() {
    priv = nullptr;
}
void Pen::set_dash_pattern(const float *, size_t) {
    NVG_UNSUPPORT(Pen::set_dash_pattern);
}
void Pen::set_dash_offset(float offset) {
    NVG_UNSUPPORT(Pen::set_dash_offset);
}
void Pen::set_dash_style(DashStyle) {
    NVG_UNSUPPORT(Pen::set_dash_style);
}
void Pen::set_line_cap(LineCap cap) {
    begin_mut();
    priv->cap = cap;
}
void Pen::set_line_join(LineJoin j) {
    begin_mut();
    priv->join = j;
}
void Pen::set_miter_limit(float li) {
    begin_mut();
    priv->miter_limit = li;
}

BTK_NS_END


// Implmentation of fontstash, just forwward the args to FontAtlas
// FONScontext is just for cap

namespace {
    FONScontext *fonsCreateInternal(FONSparams *param) {
        return new FontAtlas(param->width, param->height);
    }
    void         fonsDeleteInternal(FONScontext *s) {
        delete static_cast<FontAtlas*>(s);
    }

    bool         fonsValidateTexture(FONScontext *s, int *dirty) {
        return static_cast<FontAtlas*>(s)->validate(dirty);
    }
    uint8_t     *fonsGetTextureData(FONScontext *s, int *width, int *height) {
        auto ctxt = static_cast<FontAtlas*>(s);
        if (width) {
            *width = ctxt->width;
        }
        if (height) {
            *height = ctxt->height;
        }
        return ctxt->bitmap.data();
    }

    void         fonsSetSpacing(FONScontext *s, float spacing) {
        static_cast<FontAtlas*>(s)->spacing = spacing;
    }
    void         fonsSetSize(FONScontext *s, float size) {
        static_cast<FontAtlas*>(s)->size = size;
    }
    void         fonsSetFont(FONScontext *s, int font) {
        static_cast<FontAtlas*>(s)->font = font;
    }
    void         fonsSetAlign(FONScontext *s, int align) {
        static_cast<FontAtlas*>(s)->align = Alignment(align);
    }

    float        fonsTextBounds(FONScontext *s, float x, float y, const char* str, const char* end, float* bounds) {
        return static_cast<FontAtlas*>(s)->text_bounds(x, y, str, end, bounds);
    }
    void         fonsLineBounds(FONScontext *s, float y, float* miny, float* maxy) {
        return static_cast<FontAtlas*>(s)->line_bounds(y, miny, maxy);
    }
    void         fonsVertMetrics(FONScontext *s, float* ascender, float* descender, float* lineh) {
        // TODO 
    }
}