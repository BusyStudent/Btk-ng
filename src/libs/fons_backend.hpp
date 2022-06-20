#pragma once

#include "fontstash.hpp"
#include <cstring>
#include <cmath>


#ifdef FONS_SDL_RENDERER

#if    FONS_CLEARTYPE && defined(__GNUC__)
    #warning "Current FONS_CLEARTYPE is experimental"
#elif  FONS_CLEARTYPE && defined(_MSC_VER)
    #pragma message("Current FONS_CLEARTYPE is experimental")
#endif

#if    SDL_VERSION_ATLEAST(2,0,10)
    #define SDL_RenderDrawPoint SDL_RenderDrawPointF
#endif

#include <SDL2/SDL_version.h>
#include <SDL2/SDL_render.h>

FONS_NS_BEGIN

//FIXME : It has a bug when the font color is changed.(if you want to avoid it,flush after draw_text)


class SDLTextRenderer : public TextRenderer {
    public:
        SDLTextRenderer(SDL_Renderer *r,Fontstash &m,int w = 512,int h = 512);
        ~SDLTextRenderer();

        void set_color(SDL_Color);
        void set_color(Color    );
    private:
        void render_update(int x,int y,int w,int h) override;
        void render_resize(int w,int h) override;
        void render_draw(const Vertex *vertices,int nvertices) override;
        void render_flush() override;

        SDL_Renderer *renderer;
        SDL_Texture  *texture;
        SDL_PixelFormat *format;
};

inline SDLTextRenderer::SDLTextRenderer(SDL_Renderer *r,Fontstash &m,int w,int h)
    : TextRenderer(m,w,h)
    , renderer(r)
    , texture(nullptr)
{
    // texture = SDL_CreateTexture(
    //     renderer,
    //     SDL_PIXELFORMAT_RGBA32,
    //     SDL_TEXTUREACCESS_STREAMING,
    //     w,
    //     h
    // );
    // Uint32 fmt = SDL_MasksToPixelFormatEnum(
    //     32,
    //     0xFF000000,
    //     0x00FF0000,
    //     0x0000FF00,
    //     0x00000000
    // );
    // format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    // SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
}
inline SDLTextRenderer::~SDLTextRenderer(){
    // SDL_DestroyTexture(texture);
    // SDL_FreeFormat(format);
}
inline void SDLTextRenderer::render_update(int x,int y,int w,int h){
    //Nothing to do
}
inline void SDLTextRenderer::render_flush(){
    //Nothing to do
}
inline void SDLTextRenderer::render_resize(int w,int h){
    // SDL_DestroyTexture(texture);
    // texture = SDL_CreateTexture(
    //     renderer,
    //     SDL_PIXELFORMAT_RGBA32,
    //     SDL_TEXTUREACCESS_STREAMING,
    //     w,
    //     h
    // );
    // SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
}
inline void SDLTextRenderer::render_draw(const Vertex *vertices,int nvertices){
    //SDL_DrawPoint version
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);

    int tex_w,tex_h;
    void *tex = get_data(&tex_w,&tex_h);
    //For all glyph and draw it
    for(int n = 0;n < nvertices;n++){
        const Vertex &vert = vertices[n];

        Pixel *src = static_cast<Pixel*>(tex);
        //Unpack color by RRGGBBAA
        uint8_t r = (vert.c >> 24) & 0xFF;
        uint8_t g = (vert.c >> 16) & 0xFF;
        uint8_t b = (vert.c >> 8) & 0xFF;
        uint8_t a =  vert.c & 0xFF;
        //For this glyph and draw point
        for(int y = 0;y < vert.glyph_h;y++){
            for(int x = 0;x < vert.glyph_w;x++){
                Pixel pixel = src[((y + vert.glyph_y) * tex_w) + (x + vert.glyph_x)];

                if(pixel == 0){
                    continue;
                }
                //Zero is transparent,if not,draw it
#if FONS_CLEARTYPE
                //TODO : ClearType(This version works bad on Black Color)
                //Unpack ClearType pixels to three GrayScale
                float ra = (pixel >> 24) & 0xFF;
                float ga = (pixel >> 16) & 0xFF;
                float ba = (pixel >> 8) & 0xFF;

                uint8_t new_r = (r * ra) / 255;
                uint8_t new_g = (g * ga) / 255;
                uint8_t new_b = (b * ba) / 255;

                SDL_SetRenderDrawColor(renderer,new_r,new_g,new_b,a);
#else
                //Gray scale mode
                uint8_t alpha = a * float(pixel) / 255.0f;
                SDL_SetRenderDrawColor(renderer,r,g,b,alpha);
#endif
                SDL_RenderDrawPoint(renderer,vert.screen_x + x,vert.screen_y + y);
            }
        }
    }


    //Do 
    // int tex_w,tex_h;
    // void *tex = get_data(&tex_w,&tex_h);

    // SDL_Color color;

    // void *pixels;
    // int   pitch;

    // //Lock !
    // if(SDL_LockTexture(texture,nullptr,&pixels,&pitch) != 0){
    //     printf("SDL_LockTexture failed: %s\n",SDL_GetError());
    //     LILIM_ASSERT(false);
    // }
    
    // for(int n = 0;n < nvertices;n++){
    //     const Vertex &vert = vertices[n];

    //     //Begin Update
    //     SDL_Rect glyph_rect;
    //     glyph_rect.x = vert.glyph_x;
    //     glyph_rect.y = vert.glyph_y;
    //     glyph_rect.w = vert.glyph_w;
    //     glyph_rect.h = vert.glyph_h;

    //     //Copy
    //     uint8_t *src = static_cast<uint8_t*>(tex);
    //     uint8_t *dst = static_cast<uint8_t*>(pixels);

    //     for(int y = 0;y < vert.glyph_h;y++){
    //         for(int x = 0;x < vert.glyph_w;x++){
    //             uint8_t * cur_dst = dst + ((y + vert.glyph_y) * pitch) + ((x + vert.glyph_x) * 4);
    //             uint8_t * cur_src = src + ((y + vert.glyph_y) * tex_w) + (x + vert.glyph_x);

    //             //Unpack color by RRGGBBAA
    //             uint8_t r = (vert.c >> 24) & 0xFF;
    //             uint8_t g = (vert.c >> 16) & 0xFF;
    //             uint8_t b = (vert.c >> 8) & 0xFF;
    //             uint8_t a = vert.c & 0xFF;

    //             //Map it to the texture
    //             a *= float(*cur_src) / 255.0f;

    //             (*(uint32_t*)cur_dst) = SDL_MapRGBA(
    //                 format,
    //                 r,
    //                 g,
    //                 b,
    //                 *cur_src
    //             );
    //         }
    //     }
    // }
    // //Update done
    // SDL_UnlockTexture(texture);

    // //Draw !
    // for(int n = 0;n < nvertices;n++){
    //     const Vertex &vert = vertices[n];

    //     SDL_Rect glyph_rect;
    //     glyph_rect.x = vert.glyph_x;
    //     glyph_rect.y = vert.glyph_y;
    //     glyph_rect.w = vert.glyph_w;
    //     glyph_rect.h = vert.glyph_h;

// #if SDL_VERSION_ATLEAST(2,0,10)
        // Use float version
        // SDL_FRect dst_rect;
        // dst_rect.x = vert.screen_x;
        // dst_rect.y = vert.screen_y;
        // dst_rect.w = vert.screen_w;
        // dst_rect.h = vert.screen_h;

        // SDL_RenderCopyF(renderer,texture,&glyph_rect,&dst_rect);
// #else
        // SDL_Rect dst_rect;
        // dst_rect.x = vert.screen_x;
        // dst_rect.y = vert.screen_y;
        // dst_rect.w = vert.screen_w;
        // dst_rect.h = vert.screen_h;

        // SDL_RenderCopy(renderer,texture,&glyph_rect,&dst_rect);
// #endif
    // }
}
inline void SDLTextRenderer::set_color(Color c){
    Context::set_color(c);
}
inline void SDLTextRenderer::set_color(SDL_Color c){
    Color pix = 0;
    //Pack to RRGGBBAA
    pix |= (c.r << 24);
    pix |= (c.g << 16);
    pix |= (c.b << 8);
    pix |= c.a;

    Context::set_color(pix);
}

FONS_NS_END

//Cleanup
#undef SDL_RenderDrawPoint

#endif


#ifdef FONS_GL_RENDERER

FONS_NS_BEGIN

class GLTextRenderer : public TextRenderer {
    public:
        GLTextRenderer(Fontstash &m,int w = 512,int h = 512);
        ~GLTextRenderer();
    private:
        GLuint vbo;
};

FONS_NS_END

#endif