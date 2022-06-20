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
#include <algorithm>
#include <climits>
#include <cstring>

#define _FONS_SOURCE_
#include "fontstash.hpp"

FONS_NS_BEGIN

#if FONS_CLEARTYPE
constexpr int bytes_per_pixels = 4;
#else
constexpr int bytes_per_pixels = 1;
#endif

Font::Font(){

}
Font::~Font(){

}

Ref<Font> Font::clone(){
    int id = stash->add_font(
        face->clone()
    );
    return stash->get_font(id);
}

Glyph *Font::get_glyph(FontParams param,int req_bitmap){
    //Clear cache if too big
    if(glyphs.size() > FONS_MAX_CACHED_GLYPHS){
        FONS_LOG("Too many glyphs in cache, clear it");
        glyphs.clear();
    }
    //Check params
    if(param.size <= 0 || param.size > FONS_MAX_FONT_SIZE){
        FONS_LOG("Invalid font size");
        return nullptr;
    }

    Glyph *g    = nullptr;
    Face  *face = nullptr;
    Uint   idx  = 0;
    auto stash = param.context->stash;
    auto ctxt  = param.context;

    //find existing glyph
    g = query_cache(param,req_bitmap);
    
    //No existing glyph, create one
    if(g == nullptr){
        if(face == nullptr){
            face = get_face(param.codepoint);
            idx  = face->glyph_index(param.codepoint);
        }
        //Get metrics
        face->set_size(param.size);

        Glyph glyph;
        auto m = face->build_glyph(idx);

        //Set glyph info
        static_cast<GlyphMetrics&>(glyph) = m;
        static_cast<FontParams&>(glyph) = param;
        //Insert
        auto iter = glyphs.insert(std::make_pair(param.codepoint,glyph));
        g = &iter->second;
    }
    //Require bitmap but not created
    if(req_bitmap && g->x < 0 && g->y < 0){
        if(face == nullptr){
            face = get_face(param.codepoint);
            idx  = face->glyph_index(param.codepoint);
        }
        //do render
        face->set_size(param.size);

        int x,y;
        auto m = face->build_glyph(idx);
        //Alloc space
        bool v = ctxt->atlas.add_rect(
            m.width,
            m.height,
            &x,
            &y
        );
        //Atlas full
        if(!v){
            //Try to slove it
            if(!ctxt->handle_atlas_full()){
                //No solution
                FONS_LOG("Fail to add glyph to atlas");
                return nullptr;
            }
            v = ctxt->atlas.add_rect(
                m.width,
                m.height,
                &x,
                &y
            );
            if(!v){
                //No solution
                FONS_LOG("Fail to add glyph to atlas");
                return nullptr;
            }
            //Sloved
            FONS_LOG("Success to slove atlas full");
        }
        //Mark the glyph position in atlas
        g->x = x;
        g->y = y;
        //Rasterize
        face->render_glyph(
            idx,
            ctxt->bitmap.data(),
            ctxt->bitmap_w * bytes_per_pixels,
            x,
            y
        );
        if(param.blur > 0){
            //TODO blur
        }
        //Update dirty
        int *dirty = ctxt->dirty_rect;
        dirty[0] = std::min(dirty[0],x);
        dirty[1] = std::min(dirty[1],y);
        dirty[2] = std::max(dirty[2],x + g->width);
        dirty[3] = std::max(dirty[3],y + g->height);
    }
    return g;
}
Face *Font::get_face(char32_t codepoint){
    Uint idx = face->glyph_index(codepoint);
    if(idx != 0){
        //Self has the glyph
        return face.get();
    }
    //Query fallback
    for(auto iter = fallbacks.begin();iter != fallbacks.end();){
        Font *f = stash->get_font(*iter);
        if(f == nullptr){
            //Unexisting font
            iter = fallbacks.erase(iter);
            continue;
        }
        idx = f->face->glyph_index(codepoint);
        if(idx != 0){
            //Found font has this glyph
            return f->face.get();
        }
        ++iter;
    }
    //Oh,no.try callback
    if(stash->get_fallback != nullptr){
        Font *f = stash->get_fallback(codepoint);
        if(f != nullptr && f != this){
            //Add it to fallbacks,faster
            fallbacks.push_back(f->get_id());
            return f->face.get();
        }
    }
    //Still no found :( ,use self
    return face.get();
}
Glyph *Font::query_cache(FontParams param,int req_bitmap){
    auto iter = glyphs.find(param.codepoint);
    auto num  = glyphs.count(param.codepoint);

    if(req_bitmap){
        while(num){
            auto &glyph = iter->second;
            //Require bitmap,so we need check the blur and context 
            //Make sure we doesnot mix different context
            if(glyph.size == param.size && 
               glyph.blur == param.blur && 
               glyph.context == param.context){
                //Found
                return &glyph;
            }
            --num;
            ++iter;
        }
    }
    else{
        //No bitmap required,just check size
        while(num){
            auto &glyph = iter->second;
            
            if(glyph.size == param.size){
                //Found
                return &glyph;
            }
            --num;
            ++iter;
        }
    }
    //Not found
    return nullptr;
}
//Clear cache of specified context
void Font::clear_cache_of(Context *ctxt){
    auto iter = glyphs.begin();
    while(iter != glyphs.end()){
        auto &glyph = iter->second;
        if(glyph.context == ctxt){
            iter = glyphs.erase(iter);
        }
        else{
            ++iter;
        }
    }
}

Fontstash::Fontstash(Manager &m){
    _manager = &m;
}
Fontstash::~Fontstash(){

}

int   Fontstash::add_font(Ref<Face> face){
    int id;
    do{
        id = std::rand();
        if(id < 0){
            id = -id;
        }
    }
    while(fonts.find(id) != fonts.end());
    Ref<Font> f = new Font();
    f->stash = this;
    f->face = face;
    f->id = id;
    fonts[id] = f;

    #if FONS_CLEARTYPE
    face->set_flags(
        FT_LOAD_TARGET_LCD 
    );
    #endif

    return id;
}
Font *Fontstash::get_font(int id){
    auto it = fonts.find(id);
    if(it == fonts.end()){
        return nullptr;
    }
    return it->second.get();
}
Font *Fontstash::get_font(const char *name){
    for(auto &it : fonts){
        if(it.second->name == name){
            return it.second.get();
        }
    }
    return nullptr;
}
void  Fontstash::remove_font(int id){
    fonts.erase(id);
}

//Context operations
Context::Context(Fontstash &m,int w,int h):atlas(m.manager(),w,h){
    stash = &m;
    bitmap_w = w;
    bitmap_h = h;
    bitmap.resize(bitmap_w * bitmap_h);

    //Push one state
    push_state();

    //Init dirty
    dirty_rect[0] = w;
    dirty_rect[1] = h;
    dirty_rect[2] = 0;
    dirty_rect[3] = 0;

    //Init Error handler
    handler = nullptr;
    user    = nullptr;
}
Context::~Context(){
    for(auto &f:fonts){
        f->clear_cache_of(this);
    }
}

Size Context::measure_text(const char *str,const char *end){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return {0,0};
    }
    if(end == nullptr){
        end = str + std::strlen(str);
    }
    Size size = {0,0};
    auto m = font->metrics_of(states.top().size);
    //Using UINT_MAX as no codepoint
    //0 means empty glyph
    bool first = true;
    Uint prev = UINT_MAX;

    while(str < end){
        char32_t c = Utf8Decode(str);
        FontParams param;
        param.context = this;
        param.codepoint = c;
        param.size = states.top().size;
        param.blur = states.top().blur;
        //Kerning / Spacing
        if(!first){
            size.width += font->kerning(param.size,prev,c);
            size.width += states.top().spacing;
        }
        else{
            first = false;
        }
        prev = c;

        //Send to fond
        Glyph *g = font->get_glyph(param,FONS_GLYPH_BITMAP_OPTIONAL);
        if(g == nullptr){
            //No Glyph?
            break;
        }
        int yoffset = m.ascender - g->bitmap_top;

        size.height = std::max(size.height,g->height);
        size.height = std::max(size.height,g->height + yoffset);
        size.width += g->advance_x;
    }
    return size;
}

void Context::vert_metrics(float* ascender, float* descender, float* lineh){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return;
    }
    auto m = font->metrics_of(states.top().size);
    
    if(ascender != nullptr){
        *ascender = m.ascender;
    }
    if(descender != nullptr){
        *descender = m.descender;
    }
    if(lineh != nullptr){
        *lineh = m.height;
    }
}
void Context::line_bounds(float y, float* miny, float* maxy){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return;
    }
    auto m = font->metrics_of(states.top().size);
    //Align To Top
    //Dst()-------
    //|
    //|
    int align = states.top().align;
    if(align & FONS_ALIGN_TOP){
        //Nothing
    }
    else if(align & FONS_ALIGN_MIDDLE){
        y -= m.height / 2;
    }
    else if(align & FONS_ALIGN_BASELINE){
        y -= m.ascender;
    }
    else if(align & FONS_ALIGN_BOTTOM){
        y -= m.height;
    }
    //Done
    if(miny != nullptr){
        *miny = y;
    }
    if(maxy != nullptr){
        *maxy = y + m.height;
    }
}
float Context::text_bounds(float x, float y, const char* str, const char* end, float* bounds){
    if(end == nullptr){
        end = str + strlen(str);
    }
    Font *font = stash->get_font(states.top().font);
    auto metrics = font->metrics_of(states.top().size);
    auto align = states.top().align;

    auto size = measure_text(str,end);
    float width = size.width;
    float height = size.height;

    //Begin Transform
    TransformByAlign(&x,&y,align,size,metrics);

    if(bounds != nullptr){
        bounds[0] = x;
        bounds[1] = y;
        bounds[2] = x + width;
        bounds[3] = y + height;
    }
    //Return width
    return width;
}
bool Context::validate(int *dirty){
    if (dirty_rect[0] < dirty_rect[2] && dirty_rect[1] < dirty_rect[3]) {
        dirty[0] = dirty_rect[0];
        dirty[1] = dirty_rect[1];
        dirty[2] = dirty_rect[2];
        dirty[3] = dirty_rect[3];
        // Reset dirty rect
        dirty_rect[0] = bitmap_w;
        dirty_rect[1] = bitmap_h;
        dirty_rect[2] = 0;
        dirty_rect[3] = 0;
        return 1;
    }
    return 0;
}
//Manage owned font
void Context::add_font(int id){
    if(id < 0){
        return;
    }
    Font *f = stash->get_font(id);
    if(f == nullptr){
        return;
    }
    fonts.emplace(f);
}
void Context::remove_font(int id){
    if(id < 0){
        return;
    }
    Font *f = stash->get_font(id);
    if(f == nullptr){
        return;
    }
    fonts.erase(f);
}
void Context::dump_info(FILE *fp){
    #ifndef FONS_NDEBUG
    if(fp == nullptr){
        return;
    }
    fprintf(fp,"Context at %p\n",this);
    fprintf(fp,"    width %d height %d\n",bitmap_w,bitmap_h);
    fprintf(fp,"    Dirty in:\n");
    fprintf(fp,"        minx [%d]\n",dirty_rect[0]);
    fprintf(fp,"        miny [%d]\n",dirty_rect[1]);
    fprintf(fp,"        maxx [%d]\n",dirty_rect[2]);
    fprintf(fp,"        maxy [%d]\n",dirty_rect[3]);
    fprintf(fp,"    Fonts:\n");
    for(auto &f : fonts){
        fprintf(fp,"        id %d => %p\n",f.get()->get_id(),f.get());
    }
    fprintf(fp,"    States:\n");
    fprintf(fp,"        font => %d\n",states.top().font);
    fprintf(fp,"        size => %f\n",states.top().size);
    fprintf(fp,"        blur => %d\n",states.top().blur);
    fprintf(fp,"        align => %d\n",states.top().align);
    fprintf(fp,"        spacing => %f\n",states.top().spacing);
    //If has dirty rect dump it
    if(dirty_rect[0] < dirty_rect[2] && dirty_rect[1] < dirty_rect[3]){
        fprintf(fp,"    Dirty in:\n");
        fprintf(fp,"        minx [%d]\n",dirty_rect[0]);
        fprintf(fp,"        miny [%d]\n",dirty_rect[1]);
        fprintf(fp,"        maxx [%d]\n",dirty_rect[2]);
        fprintf(fp,"        maxy [%d]\n",dirty_rect[3]);
        //Print texture data in dirty rect
        fprintf(fp,"    Texture data:\n");
        for(int y = dirty_rect[1]; y < dirty_rect[3]; y++){
            fprintf(fp,"        ");
            for(int x = dirty_rect[0]; x < dirty_rect[2]; x++){
                fprintf(fp,"%c",bitmap[y * bitmap_w + x] ? '#' : ' ');
            }
            fprintf(fp,"\n");
        }
    }
    else{
        //Dirty rect is empty
        fprintf(fp,"    DirtyRect empty:\n");
    }
    #endif
}
//Error Handler
bool Context::handle_atlas_full(){
    if(!handler){
        //No handler
        return false;
    }
    return handler(user,FONS_ATLAS_FULL,0);
}

//Atlas from nanovg
Context::Atlas::Atlas(Manager *m,int w,int h){
    // Allocate memory for the font stash.
    int n = 255;
    std::memset(this, 0, sizeof(Atlas));
    
    manager = m;
    width = w;
    height = h;

    // Allocate space for skyline nodes
    nodes = (AtlasNode*)manager->malloc(sizeof(AtlasNode) * n);
    memset(nodes, 0, sizeof(AtlasNode) * n);
    nnodes = 0;
    cnodes = n;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
Context::Atlas::~Atlas(){
    manager->free(nodes);
}
bool Context::Atlas::insert_node(int idx,int x,int y,int w){
    int i;
    // Insert node
    if (nnodes+1 > cnodes) {
        cnodes = cnodes == 0 ? 8 : cnodes * 2;
        nodes = (AtlasNode*)manager->realloc(nodes, sizeof(AtlasNode) * cnodes);
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
void Context::Atlas::remove_node(int idx){
    int i;
    if (nnodes == 0) return;
    for (i = idx; i < nnodes-1; i++)
        nodes[i] = nodes[i+1];
    nnodes--;
}
void Context::Atlas::expend(int w,int h){
    // Insert node for empty space
    if (w > width)
        insert_node(nnodes, width, 0, w - width);
    width = w;
    height = h;
}
void Context::Atlas::reset(int w,int h){
    width = w;
    height = h;
    nnodes = 0;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
bool Context::Atlas::add_skyline(int idx, int x, int y, int w, int h){
    int i;

    // Insert new node
    if (insert_node(idx, x, y+h, w) == 0)
        return false;

    // Delete skyline segments that fall under the shadow of the new segment.
    for (i = idx+1; i < nnodes; i++) {
        if (nodes[i].x < nodes[i-1].x + nodes[i-1].width) {
            int shrink = nodes[i-1].x + nodes[i-1].width - nodes[i].x;
            nodes[i].x += (short)shrink;
            nodes[i].width -= (short)shrink;
            if (nodes[i].width <= 0) {
                remove_node(i);
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
            remove_node(i+1);
            i--;
        }
    }

    return true;
}
int  Context::Atlas::rect_fits(int i, int w, int h){
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
bool Context::Atlas::add_rect(int rw, int rh, int* rx, int* ry){

    int besth = height, bestw = width, besti = -1;
    int bestx = -1, besty = -1, i;

    // Bottom left fit heuristic.
    for (i = 0; i < nnodes; i++) {
        int y = rect_fits(i, rw, rh);
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
    if (add_skyline(besti, bestx, besty, rw, rh) == 0)
        return false;

    *rx = bestx;
    *ry = besty;

    return 1;
}

void Context::get_atlas_size(int *w,int *h){
    *w = bitmap_w;
    *h = bitmap_h;
}
void Context::expand_atlas(int w,int h){
    w = std::max(w,bitmap_w);
    h = std::max(h,bitmap_h);

    if(w == bitmap_w && h == bitmap_h){
        return;
    }
    
    std::vector<Pixel> new_map(w * h);
    //Copy old map to new map
    for(int y = 0;y < bitmap_h;y++){
        for(int x = 0;x < bitmap_w;x++){
            new_map[y * w + x] = bitmap[y * bitmap_w + x];
        }
    }
    //Increase atlas size
    atlas.expend(w,h);

    //Mark as dirty
    short maxy = 0;
    for(int i = 0;i < atlas.nnodes;i++){
        maxy = std::max(maxy,atlas.nodes[i].y);
    }

    dirty_rect[0] = 0;
    dirty_rect[1] = 0;
    dirty_rect[2] = bitmap_w;
    dirty_rect[3] = maxy;

    bitmap = std::move(new_map);
    bitmap_w = w;
    bitmap_h = h;
}
void Context::reset_atlas(int w,int h){
    bitmap.resize(w * h);
    atlas.reset(w,h);
    bitmap_w = w;
    bitmap_h = h;
    dirty_rect[0] = w;
    dirty_rect[1] = h;
    dirty_rect[2] = 0;
    dirty_rect[3] = 0;

    //TODO Reset Glyph in owned font
    //Robustness: Get current draw font and reset it
    int id  = states.top().font;
    Font *ft = stash->get_font(id);
    if(ft != nullptr){
            ft->clear_cache_of(this);
    }

    for(auto &f:fonts){
        Font *font = f.get();
        if(font == ft){
            //Already reset , skip :)
            continue;
        }
        font->clear_cache_of(this);
    }
}

void TransformByAlign(
    float *x,float *y,
    int align,
    Size size,
    FaceMetrics m
){
    //Align To Top
    //Dst()-------
    //|
    //|
    float ix = *x;
    float iy = *y;
    if(align & FONS_ALIGN_TOP){
        //Nothing
    }
    else if(align & FONS_ALIGN_MIDDLE){
        iy -= size.height / 2;
    }
    else if(align & FONS_ALIGN_BASELINE){
        iy -= size.height / m.height * m.ascender;
    }
    else if(align & FONS_ALIGN_BOTTOM){
        iy -= size.height;
    }

    //X
    if(align & FONS_ALIGN_LEFT){
        //Nothing
    }
    else if(align & FONS_ALIGN_CENTER){
        ix -= size.width / 2;
    }
    else if(align & FONS_ALIGN_RIGHT){
        ix -= size.width;
    }
    *x = ix;
    *y = iy;
}

#ifndef FONS_MINI
//TextRenderer
TextRenderer::TextRenderer(Fontstash &s,int w,int h):Context(s,w,h){
    set_error_handler([](void *self,int code,int) -> bool{
        if(code != FONS_ATLAS_FULL){
            return false;//Unhandled error
        }
        //Resize
        TextRenderer *t = (TextRenderer*)self;
        int w,h;
        t->get_atlas_size(&w,&h);
        t->expand(
            w * 2,
            h * 2
        );
        
        FONS_LOG("Resize atlas to %d,%d",w,h);

        return true;
    },this);
}
TextRenderer::~TextRenderer(){
    manager()->free(text_buffer);
}
void TextRenderer::draw_text(float x,float y,const char *str,const char *end){
    if(end == nullptr){
        end = str + std::strlen(str);
    }
    auto font = stash->get_font(states.top().font);
    if(font == nullptr){
        return;
    }
    //Get size
    auto size = measure_text(str,end);
    //Begin iterator
    auto m = font->metrics_of(states.top().size);
    //Is first?
    bool first = true;
    Uint prev = 0;

    //Transform back
    TransformByAlign(
        &x,&y,
        states.top().align,
        size,
        m
    );

    while(str < end){
        char32_t c = Utf8Decode(str);
        FontParams param;
        param.context = this;
        param.codepoint = c;
        param.size = states.top().size;
        param.blur = states.top().blur;
        //Kerning / Spacing
        if(!first){
            x += font->kerning(param.size,prev,c);
            x += states.top().spacing;
        }
        else{
            first = false;
        }
        prev = c;

        //Send to find
        Glyph *g = font->get_glyph(param,FONS_GLYPH_BITMAP_REQUIRED);
        if(g == nullptr){
            //No Glyph?
            break;
        }
        float yoffset = m.ascender - g->bitmap_top;
        float xoffset = g->bitmap_left;

        //Make vert
        Vertex vert;

        vert.glyph_x = g->x;
        vert.glyph_y = g->y;
        vert.glyph_w = g->width;
        vert.glyph_h = g->height;

        //Screen dst
        vert.screen_x = x + xoffset;
        vert.screen_y = y + yoffset;
        vert.screen_w = g->width;
        vert.screen_h = g->height;

        vert.c = states.top().color;

        //Add vert
        add_vert(vert);

        //Move forward
        x += g->advance_x;
    }
}
void TextRenderer::flush(){
    if(has_dirty()){
        //Notify update dirty

        int dirty[4];
        validate(dirty);

        int x = dirty[0];
        int y = dirty[1];
        int w = dirty[2] - dirty[0];
        int h = dirty[3] - dirty[1];

        render_update(x,y,w,h);
    }
    if(!vertices.empty()){
        //Let renderer draw

        render_draw(vertices.data(),vertices.size());
        render_flush();
        vertices.clear();
    }
}
void TextRenderer::draw_vtext(float x,float y,const char *fmt,...){
    if(fmt == nullptr){
        return;
    }
    size_t req_size;
    va_list args;
    va_start(args,fmt);
#ifdef _WIN32
    req_size = _vscprintf(fmt,args);
#else
    req_size = vsnprintf(nullptr,0,fmt,args);
#endif
    va_end(args);

    //Allocate buffer if needed
    if(req_size > text_length){
        text_buffer = static_cast<char*>(manager()->realloc(
            text_buffer,
            req_size + 1
        ));
        text_length = req_size;
    }
    else if(req_size < text_length / 2){
        //Decrease buffer size
        text_buffer = static_cast<char*>(manager()->realloc(
            text_buffer,
            text_length / 2 + 1
        ));
        text_length /= 2;
    }

    va_start(args,fmt);
    vsprintf(text_buffer,fmt,args);
    va_end(args);

    //Draw
    draw_text(x,y,text_buffer,text_buffer + req_size);
}

void TextRenderer::add_vert(const Vertex &vert){
    vertices.push_back(vert);
}
void TextRenderer::expand(int w,int h){
    //First flush the current vertices
    flush();
    //Expand the atlas
    Context::expand_atlas(w,h);
    //Notify the render
    render_resize(w,h);
}
void TextRenderer::reset(int w,int h){
    //First flush the current vertices
    flush();
    //Reset the atlas
    Context::reset_atlas(w,h);
    //Notify the render
    render_resize(w,h);
}
#endif

//Text Iterator
bool TextIter::init(Context *ctxt,float x,float y,const char* str,const char* end,int bitmapOption){
    //Zero first
    std::memset(this,0,sizeof(TextIter));

    if(end == nullptr){
        end = str + strlen(str);
    }
    auto &state = ctxt->states.top();

    //Measure
    this->font = ctxt->stash->get_font(
        state.font
    );
    if(this->font == nullptr){
        return false;
    }
    auto size = ctxt->measure_text(str,end);

    this->width = size.width;
    this->height = size.height;
    //Transform back
    this->metrics = font->metrics_of(state.size);
    TransformByAlign(&x,&y,state.align,size,metrics);


    this->x = this->nextx = x;
    this->y = this->nexty = y;
    this->spacing = state.spacing;
    this->str = str;
    this->next = str;
    this->end = end;
    this->codepoint = 0;
    this->prevGlyphIndex = -1;
    this->bitmapOption = bitmapOption;
    this->context = ctxt;

    this->iblur = state.blur;
    this->isize = state.size;
    this->spacing = state.spacing;

    #ifndef FONS_NDEBUG
    printf("ascender %f\n",metrics.ascender);
    #endif

    return true;
}

bool TextIter::to_next(Quad *quad){
    Glyph *glyph;
    if(str == end){
        return false;
    }
    char32_t ch = Utf8Decode(str);

    FontParams params;
    params.codepoint = ch;
    params.blur = iblur;
    params.size = isize;
    params.context = context;

    glyph = font->get_glyph(params,bitmapOption);
    if(glyph == nullptr){
        //No glyph :(
        //Accroding to orginal code,we should set prev into -1
        prevGlyphIndex = -1;
        return true;
    }

    if(prevGlyphIndex != -1){
        //Add kerning and spacing
        nextx  += font->kerning(params.size,prevGlyphIndex,ch);
        nextx  += spacing;
    }
    prevGlyphIndex = ch;

    x = nextx;
    y = nexty;

    //Generate quad
    float itw = 1.0f / context->bitmap_w;
    float ith = 1.0f / context->bitmap_h;
    //Mark glyph output
    float y_offset = metrics.ascender - glyph->bitmap_top;
    float x_offset = glyph->bitmap_left;
    float act_x = x + x_offset;
    float act_y = y + y_offset;

    quad->x0 = act_x;
    quad->x1 = act_x + glyph->width;
    quad->y0 = act_y;
    quad->y1 = act_y + glyph->height;

    //Mark glyph source
    quad->s0 = glyph->x * itw;
    quad->t0 = glyph->y * ith;
    quad->s1 = (glyph->x + glyph->width) * itw;
    quad->t1 = (glyph->y + glyph->height)* ith;

    //Debug print
    #ifndef FONS_NDEBUG
    printf("glyph pen_y %f bitmap_top %d height %d width %d advance_x %d\n",
        y_offset,
        glyph->bitmap_top,
        glyph->height,
        glyph->width,
        glyph->advance_x
    );
    #endif
    //Move forward
    nextx += glyph->advance_x;

    return true;
}

FONS_NS_END

//For nanovg C Interface

#ifndef FONS_MACRO_WRAPPER

static thread_local FONSruntime *_fons_runtime = nullptr;

FONS_CAPI(FONSruntime *) fonsCreateRuntime(Lilim_Manager *manager){
    using namespace FONS_NAMESPACE;
    if(manager == nullptr){
        return nullptr;
    }
    return new FONSruntime(reinterpret_cast<Manager&>(*manager));
}
FONS_CAPI(void         ) fonsDeleteRuntime(FONSruntime *stash){
    delete stash;
}
FONS_CAPI(void         ) fonsMakeCurrent(FONSruntime *stash){
    _fons_runtime = stash;
}

FONS_CAPI(FONScontext *) fonsCreateInternal(FONSparams* params){
    if(_fons_runtime == nullptr){
        #ifndef FONS_NDEBUG
        fprintf(stderr,"fonsCreateInternal: No current FONSruntime\n");
        fprintf(stderr,"fonsCreateInternal: Please call fonsMakeCurrent() first\n");
        #endif
        return nullptr;
    }
    return new FONScontext(*_fons_runtime,params->width,params->height);
}
FONS_CAPI(void         ) fonsDeleteInternal(FONScontext* s){
    return delete s;
}

FONS_CAPI(void         ) fonsSetSpacing(FONScontext *s,float spacing){
    return s->set_spacing(spacing);
}
FONS_CAPI(void         ) fonsSetSize(FONScontext *s,float size){
    return s->set_size(size);
}
FONS_CAPI(void         ) fonsSetFont(FONScontext *s,int font){
    return s->set_font(font);
}
FONS_CAPI(void         ) fonsSetBlur(FONScontext *s,int blur){
    return s->set_blur(blur);
}
FONS_CAPI(void         ) fonsSetAlign(FONScontext *s,int align){
    return s->set_align(align);
}

// States Manage
FONS_CAPI(void         ) fonsPushState(FONScontext *s){
    return s->push_state();
}
FONS_CAPI(void         ) fonsPopState(FONScontext *s){
    return s->pop_state();
}
FONS_CAPI(void         ) fonsClearState(FONScontext *s){
    return s->clear_state();
}

// Font Set / Add
FONS_CAPI(int          ) fonsAddFont(FONScontext *s,const char* name,const char* path,int fontindex){
    auto stash   = s->fontstash();
    auto manager = stash->manager();

    auto face = manager->new_face(path,fontindex);
    if(face.empty()){
        return FONS_INVALID;
    }
    int id = stash->add_font(face);
    if(id != FONS_INVALID && name != nullptr){
        stash->get_font(id)->set_name(name);
    }
    return id;
}
FONS_CAPI(int          ) fonsAddFontMem(FONScontext *s,const char* name,unsigned char* data,int ndata,int freeData,int fontindex){
    using namespace FONS_NAMESPACE;
    
    auto stash   = s->fontstash();
    auto manager = stash->manager();

    Ref<Blob> blob = new Blob(data,ndata);
    if(freeData){
        blob->set_finalizer([](void *ptr,size_t,void*){
            std::free(ptr);
        },nullptr);
    }

    auto face = manager->new_face(blob,fontindex);
    if(face.empty()){
        return FONS_INVALID;
    }
    int id = stash->add_font(face);
    if(id != FONS_INVALID && name != nullptr){
        stash->get_font(id)->set_name(name);
    }
    return id;
}
FONS_CAPI(int          ) fonsGetFontByName(FONScontext *s,const char* name){
    auto stash = s->fontstash();
    auto font  = stash->get_font(name);
    if(font == nullptr){
        return FONS_INVALID;
    }
    return font->get_id();
}
FONS_CAPI(int          ) fonsAddFallbackFont(FONScontext *s,int font,int fontindex){
    auto stash = s->fontstash();
    auto f = stash->get_font(font);
    if(f == nullptr){
        return false;
    }
    f->add_fallback(fontindex);
    return true;
}
FONS_CAPI(void         ) fonsResetFallbackFont(FONScontext *s,int font){
    auto stash = s->fontstash();
    auto f = stash->get_font(font);
    if(f == nullptr){
        return;
    }
    f->reset_fallbacks();
}

// Measure Text
FONS_CAPI(float        ) fonsTextBounds(FONScontext *s,float x,float y,const char* str,const char* end,float* bounds){
    return s->text_bounds(x,y,str,end,bounds);
}
FONS_CAPI(void         ) fonsLineBounds(FONScontext *s,float y,float* miny,float* maxy){
    return s->line_bounds(y,miny,maxy);
}
FONS_CAPI(void         ) fonsVertMetrics(FONScontext *s,float* ascender,float* descender,float* lineh){
    return s->vert_metrics(ascender,descender,lineh);
}

// Text Iterator
FONS_CAPI(int          ) fonsTextIterInit(FONScontext *s,FONStextIter* iter,float x,float y,const char* str,const char* end,int bitmapOption){
    return iter->init(s,x,y,str,end,bitmapOption);
}
FONS_CAPI(int          ) fonsTextIterNext(FONScontext *s,FONStextIter* iter,FONSquad* quad){
    return iter->to_next(quad);
}

// Pull Data
FONS_CAPI(FONS_PIXEL  *) fonsGetTextureData(FONScontext *s,int* width,int* height){
    return (FONS_PIXEL*)s->get_data(width,height);    
}
FONS_CAPI(int          ) fonsValidateTexture(FONScontext *s,int *dirty){
    return s->validate(dirty);
}

// Atlas
FONS_CAPI(void         ) fonsExpandAtlas(FONScontext *s,int width,int height){
    return s->expand_atlas(width,height);
}
FONS_CAPI(void         ) fonsResetAtlas(FONScontext *s,int width,int height){
    return s->reset_atlas(width,height);
}

#endif