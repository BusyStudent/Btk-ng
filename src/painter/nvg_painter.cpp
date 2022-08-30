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

namespace {
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

    FONScontext *fonsCreateInternal(FONSparams *param);
    void         fonsDeleteInternal(FONScontext *ctx);

    uint8_t     *fonsGetTextureData(FONScontext *s, int* width, int* height);
    bool         fonsValidateTexture(FONScontext *s, int *dirty);

    void         fonsExpandAtlas(FONScontext *s, int width, int height);
    void         fonsResetAtlas(FONScontext *s, int width, int height);

    void         fonsSetSpacing(FONScontext *s, float spacing);
    void         fonsSetSize(FONScontext *s, float size);
    void         fonsSetFont(FONScontext *s, int font);
    void         fonsSetBlur(FONScontext *s, int blur);
    void         fonsSetAlign(FONScontext *s, int align);

    float        fonsTextBounds(FONScontext *s, float x, float y, const char* str, const char* end, float* bounds);
    void         fonsLineBounds(FONScontext *s, float y, float* miny, float* maxy);
    void         fonsVertMetrics(FONScontext *s, float* ascender, float* descender, float* lineh);

    bool         fonsTextIterInit(FONScontext *s, FONStextIter* iter, float x, float y, const char* str, const char* end, int bitmapOption);
    bool         fonsTextIterNext(FONScontext *s, FONStextIter* iter, FONSquad* quad);

    FONS_DROP   (fonsAddFont);
    FONS_DROP   (fonsAddFontMem);
    FONS_DROP   (fonsAddFallbackFont);
    FONS_DROP   (fonsAddFallbackFontId);
    FONS_DROP   (fonsResetFallbackFont);
    FONS_DROP   (fonsGetFontByName);
}

// Import nanovg
#define  NVG_NO_STB
#define  NANOVG_GL_IMPLEMENTATION
#define  NANOVG_GLES3
#include "libs/nanovg_gl.hpp"
#include "libs/nanovg.hpp"
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
};
class FontGlyph {
    public:
        GlyphMetrics m; //< Glyph Metrics
        PhyFont  *font; //< Font
        short x, y; //< In bitmap position
};

class PhyBlob : public Refable<PhyBlob>, public UnCopyable {
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

class PhyFont : public Refable<PhyFont>, public UnCopyable {
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

        Ref<PhyBlob>   blob = {};
        FT_Face        face = {};
        FontContext   *ctxt = {};
        int             id  = {};
};

// For cache Glyph
class FontAtlas : public FONScontext, public UnCopyable {
    public:
        FontAtlas() = default;
        FontAtlas(int w, int h) : bitmap(w * h), width(w), height(h) {}
        ~FontAtlas() = default;

        std::vector<uint8_t> bitmap; //< Gray scale bitmap
        int   width;
        int   height;
        
        float size    = 0;
        float spacing = 0;

        float xdpi    = 72;
        float ydpi    = 72;

        Alignment align = Alignment::Left | Alignment::Top;

        // Font used in this atlas
        std::vector<WeakRef<PhyFont>> fonts;
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
        std::vector<PainterPathNode> nodes;
};

class BrushImpl   : public Refable<BrushImpl>, public Trackable {
    public:

};
class FontImpl    : public Refable<FontImpl> {
    public:
        u8string     name; //< Logical name, System will match with name
        Ref<PhyFont> font;
        float        size;
        bool         bold;
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


BTK_NS_END