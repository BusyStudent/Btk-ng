#pragma once

#ifndef LILIM_NAMESPACE
    #define LILIM_NAMESPACE Lilim
#endif

#ifdef LILIM_STATIC
    #define LILIM_NS_BEGIN \
        namespace { \
        namespace LILIM_NAMESPACE {
    #define LILIM_NS_END \
        } \
        }
#else
    #define LILIM_NS_BEGIN \
        namespace LILIM_NAMESPACE {
    #define LILIM_NS_END \
        }
#endif


#ifndef LILIM_STBTRUETYPE
    #include <ft2build.h>
    #include FT_FREETYPE_H
    #include FT_OUTLINE_H
    #include FT_ERRORS_H
    #include FT_GLYPH_H
#else
    #define FT_Face _lilim_fontinfo*
    #define FT_Library void        *
    struct _lilim_fontinfo;
#endif

#ifndef LILIM_ASSERT
    #define LILIM_ASSERT(x) assert(x)
    #include <cassert>
#endif

#ifndef LILIM_UNUSED
    #define LILIM_UNUSED(x) (void)(x)
#endif

#include <cstdlib>
#include <cstdint>
#include <cstdio>


LILIM_NS_BEGIN

using Uint = unsigned int;
using Int  = int;

class Manager;
class Face;
// Bultin refcounter
template<class T>
class Refable {
    public:
        Refable() = default;
        Refable(const Refable &) = delete;
        ~Refable() = default;

        void ref() noexcept{
            ++refcount;
        }
        void unref() noexcept{
            if(--refcount == 0){
                delete static_cast<T*>(this);
            }
        }
    private:
        int refcount = 0;
};
// Managed refcounter
template<class T>
class Ref {
    public:
        Ref() = default;
        Ref(T *p){
            ptr = p;
            ptr->ref();
        }
        Ref(const Ref &r){
            ptr = r.ptr;
            if(ptr != nullptr){
                ptr->ref();
            }
        }
        Ref(Ref &&r){
            ptr = r.ptr;
            r.ptr = nullptr;
        }
        ~Ref(){
            release();
        }

        Ref &operator=(const Ref &r){
            if(this != &r){
                release();
                ptr = r.ptr;
                if(ptr != nullptr){
                    ptr->ref();
                }
            }
            return *this;
        }
        Ref &operator=(Ref &&r){
            if(this != &r){
                release();
                ptr = r.ptr;
                r.ptr = nullptr;
            }
            return *this;
        }
        T *get() const{
            return ptr;
        }
        T *operator->() const{
            return ptr;
        }
        T &operator *() const{
            return *ptr;
        }

        void release(){
            if(ptr != nullptr){
                ptr->unref();
                ptr = nullptr;
            }
        }
        bool empty() const noexcept {
            return ptr == nullptr;
        }
        operator bool() const noexcept {
            return ptr != nullptr;
        }
    private:
        T *ptr = nullptr;
};

/**
 * @brief The Finalizer to finalize a Blob
 * 
 */
typedef void (*BlobFinalizer)(void *data,size_t size,void *user);

/**
 * @brief The Binary Object class 
 * 
 */
class Blob: public Refable<Blob> {
    public:
        Blob() = default;
        Blob(void *data,size_t size): _data(data), _size(size){}
        Blob(const Blob &) = delete;
        ~Blob(){
            if(_finalizer != nullptr){
                _finalizer(_data,_size,_user);
            }
        }
        void set_finalizer(BlobFinalizer f,void *user){
            _finalizer = f;
            _user = user;
        }

        void *data() const{
            return _data;
        }
        size_t size() const{
            return _size;
        }
        template<class T>
        T   *as() const{
            return static_cast<T*>(_data);
        }
    private:
        BlobFinalizer _finalizer = nullptr;
        void *_user = nullptr;
        void *_data = nullptr;
        size_t _size = 0;
};

typedef void *(*ReallocFunc)(void *ptr  ,size_t size,void *user);
typedef void *(*MallocFunc )(size_t size,void *user);
typedef void  (*FreeFunc   )(void *ptr  ,void *user);

/**
 * @brief Memory Allocator
 * 
 */
class MemHandler {
    public:
        ReallocFunc realloc = nullptr;
        MallocFunc  malloc = nullptr;
        FreeFunc    free = nullptr;
        void       *user = nullptr;
};

/**
 * @brief Resource manager
 * 
 */
class Manager {
    public:
        Manager();
        Manager(const MemHandler &);
        Manager(const Manager    &) = delete;
        ~Manager();

        Ref<Face> new_face(Ref<Blob> blob,int index);
        Ref<Face> new_face(const char *file,int index);
        Ref<Blob> alloc_blob(size_t size);
        Ref<Blob> realloc_blob(Ref<Blob> blob,size_t size);

        void     *malloc (size_t size);
        void     *realloc(void *ptr,size_t size);
        void      free   (void *ptr);

        template<class T>
        T        *alloc (){
            return static_cast<T*>(malloc(sizeof(T)));
        }

        FT_Library native_handle() const{
            return library;
        }
    private:
        FT_Library library;
        MemHandler memory;
};

/**
 * @brief The description of a glyph (scaled by size)
 * 
 */
class GlyphMetrics {
    public:
        int width;
        int height;
        int bitmap_left;
        int bitmap_top;
        int advance_x;
};
/**
 * @brief Size of a face (with dpi)
 * 
 */
class FaceSize {
    public:
        float height = 0;
        float width = 0;
        float ydpi = 0;
        float xdpi = 0;
};
/**
 * @brief Size
 * 
 */
class Size {
    public:
        int width = 0;
        int height = 0;
};
/**
 * @brief The description of a font face (scaled by size)
 * 
 */
class FaceMetrics {
    public:
        float ascender = 0;
        float descender = 0;
        float height = 0;
        float max_advance = 0;
        float underline_position = 0;
        float underline_thickness = 0;
};

class Bitmap : public Size {
    public:
        Ref<Blob> data;
        Blob *operator ->() const{
            return data.get();
        }
};

enum Style : Uint {
    Normal    = 0,
    Bold      = 1 << 0,
    Italic    = 1 << 1,
    Underline = 1 << 2,
    Strikeout = 1 << 3,
};
// Clear type hinting
enum LCD : Uint {
    LCDNone       = 0,
    LCDVertical   = 1 << 0,
    LCDHorizontal = 1 << 1,
};

/**
 * @brief Face Interface
 * 
 */
class Face: public Refable<Face> {
    public:
        Face(const Face &) = delete;
        ~Face();

        Ref<Face> clone();
        FT_Face   native_handle() const{
            return face;
        }
        Uint      load_flags() const{
            return flags;
        }

        void  set_dpi    (Uint xdpi,Uint ydpi);
        void  set_size   (FaceSize size);
        void  set_size   (Uint     size);
        void  set_style  (Uint     style);
        void  set_flags  (Uint     flags);
        /**
         * @brief Get kerning of two glyphs
         * 
         * @param left The left glyph
         * @param right The right glyph
         * @return The kerning distance
         */
        Int   kerning    (Uint left,Uint right);
        /**
         * @brief Convert a UTF32 codepoint to a glyph index
         * 
         * @param codepoint 
         * @return Index (0 on not founded)
         */
        Uint  glyph_index(char32_t codepoint);

        auto  metrics()              -> FaceMetrics;
        auto  build_glyph(Uint code) -> GlyphMetrics;
        /**
         * @brief Render a glyph at the given position
         * 
         * @param code The Glyph Index
         * @param buffer The buffer to render to
         * @param pitch The buffer pitch
         * @param pen_x The pen x position
         * @param pen_y The pen y position
         */
        auto  render_glyph(
            Uint code,
            void *buffer,
            int pitch,
            int pen_x,
            int pen_y
        ) -> void;
        /**
         * @brief Get Size of a string
         * 
         * @param text The UTF8 text begin
         * @param end The UTF8 text end(nullptr on null terminated)
         * @return Size 
         */
        auto measure_text(const char *text,const char *end = nullptr) -> Size;
        /**
         * @brief Render a string
         * 
         * @param text The UTF8 text begin
         * @param end The UTF8 text end(nullptr on null terminated)
         * @return Bitmap 
         */
        auto render_text(const char *text,const char *end = nullptr) -> Bitmap;
    private:
        Face();

        Manager  *manager;
        Ref<Blob> blob;
        FT_Face   face;
        Uint      styles; // Style
        Uint      flags; // FT_LOAD_XXX
        Uint      xdpi; // DPI in set_size(Uint)
        Uint      ydpi; // DPI in set_size(Uint)
        Uint      idx; // Face Index 
    friend class Manager;
};

//Math Utility
inline int64_t  FixedFloor(int64_t x);
inline int64_t  FixedCeil(int64_t x);
//Utility
/**
 * @brief Decode UTF8 and move the pointer to the next char
 * 
 * @note It would change the input pointer
 * @param str The pointer of the UTF8 string
 * @return UTF32 codepoint
 */
extern char32_t  Utf8Decode(const char *&str);
/**
 * @brief Load a file from disk
 * 
 * @param path The UTF8 file path
 * @return Ref<Blob> (empty on error)
 */
extern Ref<Blob> MapFile(const char *path);

//Inline implement
inline int64_t FixedFloor(int64_t x){
    return ((x & -64) / 64);
}
inline int64_t FixedCeil(int64_t x){
    return (((x + 63) & -64) / 64);
}

//Manager
inline void *Manager::malloc(size_t byte){
    return memory.malloc(byte,memory.user);
}
inline void *Manager::realloc(void *ptr,size_t byte){
    return memory.realloc(ptr,byte,memory.user);
}
inline void  Manager::free(void *ptr){
    return memory.free(ptr,memory.user);
}

//Face
inline void Face::set_flags(Uint flags){
    this->flags = flags;
}
inline void Face::set_dpi(Uint xdpi,Uint ydpi){
    this->xdpi = xdpi;
    this->ydpi = ydpi;
}

LILIM_NS_END

//C-style wrapper

#ifdef LILIM_CSTATIC
    #define LILIM_CAPI(X) static     X
#else
    #define LILIM_CAPI(X) extern "C" X 
#endif

#ifndef _LILIM_SOURCE_
    #define LILIM_HANDLE(X) typedef struct _Lilim_##X  *Lilim_##X
#else
    #define LILIM_HANDLE(X) typedef LILIM_NAMESPACE::X *Lilim_##X
#endif

LILIM_HANDLE (Manager);
LILIM_HANDLE (Blob);
LILIM_HANDLE (Face);

//Manager
LILIM_CAPI(Lilim_Manager) Lilim_NewManager();
LILIM_CAPI(void         ) Lilim_DestroyManager(Lilim_Manager manager);

//Face
LILIM_CAPI(Lilim_Face   ) Lilim_NewFace(Lilim_Manager manager,Lilim_Blob blob,int index);
LILIM_CAPI(Lilim_Face   ) Lilim_RefFace(Lilim_Face face);
LILIM_CAPI(Lilim_Face   ) Lilim_CloneFace(Lilim_Face face);
LILIM_CAPI(void         ) Lilim_CloseFace(Lilim_Face face);

//Blob
LILIM_CAPI(Lilim_Blob   ) Lilim_MapData(const void *data,size_t size);
LILIM_CAPI(Lilim_Blob   ) Lilim_MapFile(const char *path);
LILIM_CAPI(Lilim_Blob   ) Lilim_RefBlob(Lilim_Blob blob);
LILIM_CAPI(void         ) Lilim_FreeBlob(Lilim_Blob blob);

//Cleanup
#undef FT_Face
#undef FT_Library