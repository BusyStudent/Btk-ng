#include "build.hpp"
#include "common.hpp"

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

// Forward declaration for nanovg
namespace {
    struct fonsContext {
        int unused;
    };
    struct fonsQuad {
        float x, y, s0, t0, s1, t1;
    };

    fonsContext *fonsCreateInternal(int flags);
    void         fonsDeleteInternal(fonsContext *ctx);    
}




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

        int  num_of_faces() const noexcept { return face->num_faces; }
        int  face_index() const noexcept { return face->face_index & 0xFFFF; }


        Ref<PhyBlob>   blob = {};
        FT_Face        face = {};
        FontContext   *ctxt = {};
        int             id  = {};
};

// For cache Glyph
class FontAtlas {
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

// Basic declation end
static FontContext *fcontext    = nullptr;
static int          fc_refcount = 0;


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
    data = std::malloc(size);
    fread(data, size, 1, fp);
    fclose(fp);

    free = [](PhyBlob *blob) {
        std::free(blob->data);
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
class FontImpl    : public Refable<FontImpl> {
    public:
        u8string     name; //< Logical name, System will match with name
        Ref<PhyFont> font;
        float        size;
        bool         bold;
};

class PainterImpl : public GLFunctions {
    public:
        PainterImpl(AbstractWindow *win);
        ~PainterImpl();

        void clear();

        AbstractWindow *win = nullptr;
        GLColor         color = Color::Black;
        GLContext      *ctxt  = nullptr;

        GLint       max_texture_size;

        FontAtlas   atlas;
};

inline PainterImpl::PainterImpl(AbstractWindow *win) {
    ctxt = static_cast<GLContext*>(win->gc_create("opengl"));
    if (!ctxt->initialize(GLFormat())) {
        BTK_THROW(std::runtime_error("Failed to initialize OpenGL context"));
    }

    // Load OpenGL functions
    load(ctxt);

    // Query info
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
}
inline PainterImpl::~PainterImpl() {
    delete ctxt;
}
inline void PainterImpl::clear() {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}



void Painter::Init() {
    if (fc_refcount == 0) {
        fcontext = new FontContext();
    }
    fc_refcount++;
}
void Painter::Shutdown() {
    if (--fc_refcount == 0) {
        delete fcontext;
        fcontext = nullptr;
    }
}
void Painter::begin() {
    priv->ctxt->begin();
}
void Painter::end() {

    // GL Cleanup
    priv->ctxt->end();
    priv->ctxt->swap_buffers();
}
void Painter::clear() {
    priv->clear();
}
Painter Painter::FromWindow(AbstractWindow *win) {
    return Painter(new PainterImpl(win));
}


BTK_NS_END