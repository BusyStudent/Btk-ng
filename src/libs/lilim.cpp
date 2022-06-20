#define _LILIM_SOURCE_
#include <algorithm>
#include <cstring>
#include "lilim.hpp"

//Platform-specific includes
#ifdef _WIN32
    #define  NOMINMAX
    #include <windows.h>
#elif defined(__linux)
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

//Stb truetype includes
#ifdef LILIM_STBTRUETYPE
    #define STB_TRUETYPE_IMPLEMENTATION
    #define STBTT_STATIC

    //Hook malloc / free
    #define STBTT_malloc(size,up)  _lilim_malloc(size,up)
    #define STBTT_free(ptr,up)     _lilim_free(ptr,up)

    static void *_lilim_malloc(size_t size,void *up){
        return static_cast<LILIM_NAMESPACE::Manager*>(up)->malloc(size);
    }
    static void  _lilim_free  (void *ptr,void *up){
        return static_cast<LILIM_NAMESPACE::Manager*>(up)->free(ptr);
    }
    
    //Include headers
    #ifndef LILIM_STBTRUETYPE_H
        #define LILIM_STBTRUETYPE_H "stb_truetype.h"
    #endif

    #include LILIM_STBTRUETYPE_H
    #include <cmath>

    //Replace some Freetype Function
    #define FT_Init_FreeType(a) 0
    #define FT_Done_FreeType(a)

    struct _lilim_fontinfo : stbtt_fontinfo {
        //Scale for current size
        float xscale;
        float yscale;
    };

#endif

LILIM_NS_BEGIN

//Manager
Manager::Manager(){
    //Init default malloc / free
    memory.realloc = reinterpret_cast<ReallocFunc>(std::realloc);
    memory.malloc = reinterpret_cast<MallocFunc>(std::malloc);
    memory.free = reinterpret_cast<FreeFunc>(std::free);
    //Initialize freetype
    if(FT_Init_FreeType(&library)){
        //TODO: Error handling
        std::abort();
    }
}
Manager::~Manager(){
    FT_Done_FreeType(library);
}

#ifndef LILIM_STBTRUETYPE
Ref<Face> Manager::new_face(Ref<Blob> blob,int index){
    FT_Bytes bytes = static_cast<FT_Bytes>(blob->data());
    FT_Long  size  = static_cast<FT_Long>(blob->size());
    FT_Face  face;
    if(FT_New_Memory_Face(library,bytes,size,index,&face)){
        //Failed to load font
        return {};
    }
    Ref<Face> ret = new Face();
    ret->manager = this;
    ret->blob    = blob;
    ret->face    = face;
    ret->idx     = index;
    return ret;
}
#else
Ref<Face> Manager::new_face(Ref<Blob> blob,int index){
    uint8_t *bytes = static_cast<uint8_t*>(blob->data());
    int      size  = static_cast<int>(blob->size());
    int     offset = stbtt_GetFontOffsetForIndex(bytes,index);
    auto     face  = alloc<_lilim_fontinfo>();
    if(offset < 0){
        //Failed to Get the offset
        free(face);
        return {};
    }
    face->userdata = this;
    if(!stbtt_InitFont(face,bytes,offset)){
        //Failed to initialize font
        free(face);
        return {};
    }
    Ref<Face> ret = new Face();
    ret->manager = this;
    ret->blob    = blob;
    ret->face    = face;
    ret->idx     = index;
    return ret;
}
#endif

Ref<Face> Manager::new_face(const char *path,int index){
    //TODO : This is not a good way to do it, we should use a better way
    //For easy implementation face clone, we use mmap to load font file
    Ref<Blob> blob = MapFile(path);
    if(blob.empty()){
        //Failed to load font
        return {};
    }
    return new_face(blob,index);
}
Ref<Blob> Manager::alloc_blob(size_t size){
    void *ptr = std::malloc(size);
    if(ptr == nullptr){
        return {};
    }
    Ref<Blob> ret = new Blob(ptr,size);
    ret->set_finalizer([](void *ptr,size_t,void *manager){
        static_cast<Manager*>(manager)->free(ptr);
    },this);
    return ret;
}

#ifndef LILIM_STBTRUETYPE
//Freetype Face
Face::Face(){
    flags = FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT;
    manager = nullptr;
    face    = nullptr;
    styles  = 0;
    xdpi    = 0;
    ydpi    = 0;
    idx     = 0;
}
Face::~Face(){
    FT_Done_Face(face);
}
void  Face::set_size(FaceSize size){
    FT_Set_Char_Size(
        face,
        size.width * 64,
        size.height * 64,
        size.xdpi,
        size.ydpi
    );
}
void  Face::set_size(Uint size){
    FT_Set_Char_Size(
        face,
        0,
        size * 64,
        xdpi,
        ydpi
    );
}
Uint  Face::glyph_index(char32_t codepoint){
    return FT_Get_Char_Index(face,codepoint);
}
Int   Face::kerning(Uint prev,Uint cur){
    FT_Vector delta;
    FT_Error err;
    err = FT_Get_Kerning(face,prev,cur,FT_KERNING_DEFAULT,&delta);
    if(err){
        //Error
        std::abort();
    }
    return delta.x >> 6;
}
auto  Face::metrics() -> FaceMetrics{
    FaceMetrics metrics;
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
auto  Face::build_glyph(Uint code) -> GlyphMetrics{
    if(FT_Load_Glyph(face,code,flags)){
        //Error
        std::abort();
    }
    FT_GlyphSlot slot = face->glyph;
    GlyphMetrics ret;

    //Get metrics
    ret.width  = slot->bitmap.width;
    ret.height = slot->bitmap.rows;
    ret.bitmap_left = slot->bitmap_left;
    ret.bitmap_top  = slot->bitmap_top;
    ret.advance_x   = slot->advance.x >> 6;

    //LCD mode
    if((flags & FT_LOAD_TARGET_LCD) == FT_LOAD_TARGET_LCD){
        LILIM_ASSERT(ret.width % 3 == 0);
        ret.width /= 3;
    }

    return ret;
}
void  Face::render_glyph(Uint code,void *b,int pitch,int pen_x,int pen_y){
    if(FT_Load_Glyph(face,code,FT_LOAD_RENDER | flags)){
        std::abort();
    }
    FT_GlyphSlot slot = face->glyph;
    //Begin rendering
    //TODO : Handle pitch and Format

    switch(slot->bitmap.pixel_mode){
        case FT_PIXEL_MODE_MONO:
        case FT_PIXEL_MODE_GRAY:{
            uint8_t *pixels = static_cast<uint8_t*>(b);
            for(int y = 0;y < slot->bitmap.rows;y++){
                for(int x = 0;x < slot->bitmap.width;x++){
                    pixels[
                        (pen_y + y) * pitch +
                        (pen_x + x)
                    ] = slot->bitmap.buffer[y * slot->bitmap.width + x];
                }
            }
            break;
        }
        case FT_PIXEL_MODE_LCD:{
            //Write to RGBA buffer
            uint8_t *pixels = static_cast<uint8_t*>(b);
            uint8_t *buffer = slot->bitmap.buffer;
            int width = slot->bitmap.width / 3;
            int row   = slot->bitmap.rows;

            for(int y = 0;y < row;y++){
                uint8_t *cur = buffer;//Current Buffer
                for(int x = 0;x < width;x++){
                    uint32_t *dst = reinterpret_cast<uint32_t*>(
                        pixels + (pen_y + y) * pitch +
                                 (pen_x + x) * 4
                    );
                    //Begin Pack
                    uint32_t pix = 0;
                    pix |= cur[0] << 24;
                    pix |= cur[1] << 16;
                    pix |= cur[2] << 8;
                    *dst = pix;

                    cur += 3;
                }
                //End Move to next row
                buffer += slot->bitmap.pitch;
            }
            break;
        }
        default:
            //Unsupported pixel mode now
            LILIM_ASSERT(false);
    }
}
#else
//Stb TrueType  Face
Face::Face(){
    flags   = 0;
    manager = nullptr;
    face    = nullptr;
    xdpi    = 0;
    ydpi    = 0;
    idx     = 0;
}
Face::~Face(){
    manager->free(face);
}
void  Face::set_size(FaceSize size){
    //Process width / height by DPI
    size.xdpi = size.xdpi ? size.xdpi : 72;
    size.ydpi = size.ydpi ? size.ydpi : 72;

    size.width *= size.xdpi / 72.0f;
    size.height *= size.ydpi / 72.0f;

    float xscale = stbtt_ScaleForPixelHeight(face,size.width);
    float yscale = stbtt_ScaleForPixelHeight(face,size.height);

    xscale  *= size.xdpi / 72.0f;
    yscale  *= size.ydpi / 72.0f;

    face->xscale = xscale;
    face->yscale = yscale;
}
void  Face::set_size(Uint size){
    FaceSize s;
    s.width = size;
    s.height = size;
    s.xdpi = xdpi;
    s.ydpi = ydpi;

    set_size(s);
}
Uint  Face::glyph_index(char32_t codepoint){
    return stbtt_FindGlyphIndex(face,codepoint);
}
Int   Face::kerning(Uint prev,Uint cur){
    return 0;
    //FIXME : Kerning bug in big size font
    // return stbtt_GetGlyphKernAdvance(face,prev,cur) * face->xscale;
}
auto  Face::metrics() -> FaceMetrics{
    FaceMetrics metrics;
    //Get metrics
    int ascender;
    int descender;
    int linegap;
    stbtt_GetFontVMetrics(face,&ascender,&descender,&linegap);
    //Scale it by size

    metrics.ascender    = std::ceil(ascender * face->yscale);
    metrics.descender   = std::ceil(descender * face->yscale);
    metrics.height      = std::ceil((ascender - descender) * face->yscale);
    metrics.max_advance = -1; // Not supported
    metrics.underline_position = -1; // Not supported
    metrics.underline_thickness = -1; // Not supported
    return metrics;
}
auto  Face::build_glyph(Uint code) -> GlyphMetrics{
    GlyphMetrics ret;
    //Get metrics
    int advance;
    int lsb;
    int x0,y0,x1,y1;

    stbtt_GetGlyphHMetrics(face,code,&advance,&lsb);
    stbtt_GetGlyphBitmapBox(face,code,face->xscale,face->yscale,&x0,&y0,&x1,&y1);

    ret.width  = x1 - x0;
    ret.height = y1 - y0;
    ret.bitmap_left = x0;
    ret.bitmap_top  = -y0;
    ret.advance_x   = advance * face->xscale;

    return ret;
}
void  Face::render_glyph(Uint code,void *b,int pitch,int pen_x,int pen_y){
    int x0,y0,x1,y1;

    stbtt_GetGlyphBitmapBox(face,code,face->xscale,face->yscale,&x0,&y0,&x1,&y1);

    int width  = x1 - x0;
    int height = y1 - y0;

    uint8_t *buffer = static_cast<uint8_t*>(b);
    uint8_t *pixels = buffer + (pen_y) * pitch + (pen_x);
    int      stride = pitch;

    //Rasterize
    stbtt_MakeGlyphBitmap(
        face,
        pixels,
        width,
        height,
        stride,
        face->xscale,
        face->yscale,
        code
    );
}
#endif

auto  Face::measure_text(const char *text,const char *end) -> Size{
    int w = 0;
    int h = 0;

    const char *cur = text;
    if(end == nullptr){
        end = text + std::strlen(text);
    }
    GlyphMetrics glyph;
    FaceMetrics  m = metrics();

    Uint prev = 0;

    while(cur != end){
        char32_t ch = Utf8Decode(cur);
        //TODO: Handle invalid utf8
        Uint idx = glyph_index(ch);
        glyph = build_glyph(idx);

        int y_offset = m.ascender - glyph.bitmap_top;

        w += glyph.advance_x;
        h = std::max(h,glyph.height);
        h = std::max(h,y_offset + glyph.height);

        //Add kerning
        if(prev != 0){
            w += kerning(prev,idx);
        }
        prev = idx;
    }
    return {w,h};
}
auto  Face::render_text(const char *text,const char *end) -> Bitmap{
    Size size = measure_text(text,end);
    auto blob = manager->alloc_blob(size.width * size.height);
    auto m    = metrics();

    uint8_t *data = static_cast<uint8_t*>(blob->data());
    //Begin rendering
    int pitch = size.width;
    int pen_x = 0;
    int pen_y = 0;

    const char *cur = text;
    if(end == nullptr){
        end = text + std::strlen(text);
    }

    Uint prev = 0;

    std::memset(data,0,size.width * size.height);

    while(cur != end){
        char32_t ch = Utf8Decode(cur);
        Uint idx = glyph_index(ch);
        GlyphMetrics glyph = build_glyph(idx);

        int y_offset = m.ascender - glyph.bitmap_top;
        int x_offset = glyph.bitmap_left;

        render_glyph(idx,data,pitch,pen_x + x_offset,pen_y + y_offset);

        //Move pen
        pen_x += glyph.advance_x;

        //Add kerning
        if(prev != 0){
            pen_x += kerning(prev,idx);
        }
        prev = idx;
    }
    Bitmap ret;
    ret.width  = size.width;
    ret.height = size.height;
    ret.data   = blob;
    return ret;
}
Ref<Face> Face::clone(){
    auto face = manager->new_face(
        blob,
        idx
    );
    face->set_dpi(xdpi,ydpi);
    face->set_flags(flags);
    return face;
}

//Utility functions    

char32_t  Utf8Decode(const char *&str){
    //Algorithm from utf8 for cpp
    auto mask = [](char ch) -> uint8_t{
        return ch & 0xff;
    };
    //Get length first
    int     len = 0;
    uint8_t lead = mask(*str);
    if(lead < 0x80){
        len = 1;
    }
    else if((lead >> 5) == 0x6){
        len = 2;
    }
    else if((lead >> 4) == 0xe){
        len = 3;
    }
    else if((lead >> 3) == 0x1e){
        len = 4;
    }
    else{
        //Invalid
        std::abort();
    }
    //Start
    char32_t ret = mask(*str);
    switch (len) {
        case 1:
            break;
        case 2:
            ++str;
            ret = ((ret << 6) & 0x7ff) + ((*str) & 0x3f);
            break;
        case 3:
            ++str; 
            ret = ((ret << 12) & 0xffff) + ((mask(*str) << 6) & 0xfff);
            ++str;
            ret += (*str) & 0x3f;
            break;
        case 4:
            ++str;
            ret = ((ret << 18) & 0x1fffff) + ((mask(*str) << 12) & 0x3ffff);                
            ++str;
            ret += (mask(*str) << 6) & 0xfff;
            ++str;
            ret += (*str) & 0x3f; 
            break;
    }
    ++str;
    return ret;
}

Ref<Blob> MapFile(const char *file){
#ifdef _WIN32
    //Convert to wide string
    size_t len = std::strlen(file);
    wchar_t *wfile = new wchar_t[len + 1];
    MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, len + 1);
    wfile[len] = 0;

    //Open file
    HANDLE fhandle = CreateFileW(
        wfile,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    delete[] wfile;
    if(fhandle == INVALID_HANDLE_VALUE){
        return {};
    }
    //Map file
    DWORD size = GetFileSize(fhandle, nullptr);
    HANDLE mhandle = CreateFileMapping(
        fhandle, 
        nullptr, 
        PAGE_READONLY, 
        0, 
        size, 
        nullptr
    );
    CloseHandle(fhandle);
    if(mhandle == nullptr){
        return {};
    }
    void *data = MapViewOfFile(mhandle, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mhandle);
    if(data == nullptr){
        return {};
    }
    //Create blob
    Ref<Blob> blob = new Blob(data, size);
    blob->set_finalizer([](void *data, size_t size, void *user){
        UnmapViewOfFile(data);
    }, nullptr);
#elif defined(__linux)
    //Mmap
    int fd = open(file,O_RDONLY);
    if(fd == -1){
        return {};
    }
    struct stat st;
    fstat(fd,&st);
    void *data = mmap(nullptr,st.st_size,PROT_READ,MAP_SHARED,fd,0);
    close(fd);
    if(data == MAP_FAILED){
        return {};
    }
    //Create blob
    Ref<Blob> blob = new Blob(data,st.st_size);
    blob->set_finalizer([](void *data, size_t size, void *user){
        munmap(data,size);
    },nullptr);
#else
    //Read all file into memory
    FILE *fp = std::fopen(file,"rb");
    std::fseek(fp,SEEK_END,0);
    auto size = std::ftell(fp);
    std::fseek(fp,SEEK_SET,0);

    void *ptr = std::malloc(size);
    if(ptr == nullptr){
        std::fclose(fp);
        return {};
    }
    Ref<Blob> blob = new Blob(ptr,size);
    blob->set_finalizer([](void *p,size_t,void*){
        std::free(p);
    },nullptr);
    //Read it
    if(std::fread(fp,ptr,size,1) != 1){
        std::fclose(fp);
        return {};
    }
    std::fclose(fp);
#endif
    return blob;
}

LILIM_NS_END


// C API
