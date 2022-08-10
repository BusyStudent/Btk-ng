#include "build.hpp"
#include "common.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(u, v) Btk_malloc(u)
#define STBTT_free(u, v)   Btk_free(u)
#define STBTT_STATIC

// Import library
#include "libs/opengl_macro.hpp"
#include "libs/stb_truetype.h"

#include <Btk/painter.hpp>
#include <Btk/context.hpp>
#include <unordered_map>
#include <filesystem>

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

class PhyBlob : public Refable<PhyBlob> {
    public:
        PhyBlob(const PhyBlob &) = delete;

        pointer_t data = nullptr;
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
    public:

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



class PainterImpl : public GLFunctions {
    public:
        PainterImpl(AbstractWindow *win);
        ~PainterImpl();

        void clear();

        GLColor     color = Color::Black;
        GLContext  *ctxt  = nullptr;

        GLint       max_texture_size;
};

PainterImpl::PainterImpl(AbstractWindow *win) {
    ctxt = static_cast<GLContext*>(win->gc_create("opengl"));
    if (!ctxt->initialize(GLFormat())) {
        BTK_THROW(std::runtime_error("Failed to initialize OpenGL context"));
    }

    // Load OpenGL functions
    load(ctxt);

    // Query info
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
}
PainterImpl::~PainterImpl() {
    delete ctxt;
}
void PainterImpl::clear() {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}



BTK_NS_END2()