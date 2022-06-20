#include "build.hpp"

#include <Btk/context.hpp>
#include <unordered_map>
#include <SDL2/SDL.h>

// Internal use helper functions

#include "sdl2_trkey.hpp"

// Using lilim as font abstraction
#if !__has_include(<ft2build.h>)
#define LILIM_STBTRUETYPE
#endif

#define LILIM_STATIC
#define FONS_MACRO_WRAPPER
#define FONS_SDL_RENDERER
#define FONS_STATIC
#include "libs/lilim.hpp"
#include "libs/lilim.cpp"
#include "libs/fontstash.cpp"
#include "libs/fons_backend.hpp"


#if defined(__gnu_linux__)
#include "common/fc_x11.hpp"
#else
#error "Unsupported platform"
#endif


BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

// Ability of SDL_Renderer

#define SDL_GC_FLOAT  SDL_VERSION_ATLEAST(2, 0, 1)
#define SDL_GC_VERTEX SDL_VERSION_ATLEAST(2, 0, 18)

// Type checker

static_assert(offsetof(SDL_Point, x) == offsetof(Point, x));
static_assert(offsetof(SDL_Point, y) == offsetof(Point, y));

static_assert(offsetof(SDL_Rect, x) == offsetof(Rect, x));
static_assert(offsetof(SDL_Rect, y) == offsetof(Rect, y));
static_assert(offsetof(SDL_Rect, w) == offsetof(Rect, w));
static_assert(offsetof(SDL_Rect, h) == offsetof(Rect, h));

// Capability of SDL_Renderer

#if SDL_GC_FLOAT
static_assert(offsetof(SDL_FPoint, x) == offsetof(FPoint, x));
static_assert(offsetof(SDL_FPoint, y) == offsetof(FPoint, y));

static_assert(offsetof(SDL_FRect, x) == offsetof(FRect, x));
static_assert(offsetof(SDL_FRect, y) == offsetof(FRect, y));
static_assert(offsetof(SDL_FRect, w) == offsetof(FRect, w));
static_assert(offsetof(SDL_FRect, h) == offsetof(FRect, h));
#endif


class SDLDriver ;
class SDLFont   ;
class SDLWindow : public AbstractWindow {
    public:
        SDLWindow(SDL_Window *w, SDLDriver *dr) : win(w), driver(dr) {}
        ~SDLWindow();

        // Overrides from WindowContext
        Size       size() const override;
        Point      position() const override;
        void       raise() override;
        void       resize(int w, int h) override;
        void       show(bool v) override;
        void       move(int x, int y) override;
        void       set_title(const char_t *title) override;
        void       set_icon(const PixBuffer &buffer) override;

        widget_t   bind_widget(widget_t widget) override;
        gcontext_t gc_create() override;

        uint32_t   id() const;
    private:
        SDL_Window *win = nullptr;
        SDLDriver  *driver = nullptr;
        Widget     *widget = nullptr;
    friend class SDLDriver;
};

class SDLDriver : public GraphicsDriver {
    public:
        SDLDriver();
        ~SDLDriver();

        // Overrides from GraphicsDriver
        window_t window_create(const char_t * title, int width, int height,WindowFlags flags) override;
        void     window_add(SDLWindow *win);
        void     window_del(SDLWindow *win);

        void     clipboard_set(const char_t *str) override;
        u8string clipboard_get() override;

        void     pump_events(UIContext *) override;

        int      sdl_event_filter(SDL_Event *event);
        void     tr_window_event(SDL_Event *evemt);

        timerid_t timer_add(Object *obj, uint32_t ms) override;
        bool      timer_del(Object *obj,timerid_t id) override;

        int       font();
    private:
        Lilim::Manager ft_manager = {}; //< Manage font
        Fons::Fontstash ft_stash  = {ft_manager};

        timestamp_t init_time;
        UIContext  *context;
        EventQueue *event_queue;
        FcContext   fc_context;

        int   def_font = -1; //< Default font id
        float xdpi;
        float ydpi;
        
        // Map ID -> AbstractWindow
        std::unordered_map<uint32_t, SDLWindow *> windows;
    friend class SDLGraphicsContext;
};

class SDLTexture : public AbstractTexture {
    public:
        SDLTexture(SDL_Texture *t, any_t ctxt) : tex(t), ctxt(ctxt) {};
        ~SDLTexture();

        // Overrides from AbstractTexture
        void  update(cpointer_t pix, int x, int y, int w, int h, int stride) override;
        Size  size() const override;
        any_t context() const override;
    private:
        SDL_Texture *tex = nullptr;
        any_t        ctxt;
    friend class SDLGraphicsContext;
};

class SDLGraphicsContext : public GraphicsContext {
    public:
        SDLGraphicsContext(SDLDriver *drv, SDL_Window *w, SDL_Renderer *r) : 
            txt_render(r, drv->ft_stash), 
            driver(drv), window(w), renderer(r) {}
        ~SDLGraphicsContext();

        // Overrides from GraphicsContext
        void draw_image(const FRect &rect, texture_t t, const Rect &src) override;
        void draw_lines(const FPoint *points, int count) override;
        void draw_points(const FPoint *points, int count) override;
        void draw_rects(const FRect *rects, int count) override;
        void fill_rects(const FRect *rects, int count) override;

        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

        void draw_text(font_t f, align_t align, u8string_view txt, float x, float y) override;

        void clear() override;
        void begin() override;
        void end() override;

        texture_t texture_create(int w, int h, PixFormat fmt) override;
        bool      set_antialias(bool v) override;

        // Features
        bool      has_feature(int f) const override;
        bool      set_feature(int f, ...) override;
        bool      get_info(int f,...) override;

        // AA func
        void aa_draw_lines(const FPoint *points, int count);

    private:
        Fons::SDLTextRenderer txt_render;
        SDLDriver    *driver = nullptr;
        SDL_Window   *window = nullptr;
        SDL_Renderer *renderer = nullptr;
        bool          antialias = true;
        float         x_scale;
        float         y_scale;

        uint8_t      *text_bitmap = nullptr;
        int           bitmap_size = 0;
};

class SDLFont   : public AbstractFont {
    public:
        SDLFont(int ft);
        ~SDLFont();

        u8string_view  name() const override;
        int            size() const override;

        // Size           measure(u8string_view txt) const override;
        bool           set_size(int sz) override;

        // Internal use
        // Size           measure(float xs, float ys, u8string_view txt) const;

        SDLFont       *clone() const;
    public:
        u8string       fname;
        int            font;
        int            fsize;
};

// Driver

SDLDriver::SDLDriver() {
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    SDL_Init(SDL_INIT_VIDEO);
    init_time = GetTicks();

    // Query DPI
    SDL_GetDisplayDPI(0, nullptr, &xdpi, &ydpi);

    printf(" xdpi : %f, ydpi : %f\n", xdpi, ydpi);
}
SDLDriver::~SDLDriver() {
    SDL_Quit();
}

int SDLDriver::sdl_event_filter(SDL_Event *event) {
    timestamp_t time = GetTicks();

    switch(event->type){
        case SDL_QUIT : {
            Event tr_event;
            tr_event.set_timestamp(GetTicks());
            tr_event.set_type(Event::Quit);
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT : {
            tr_window_event(event);
            break;
        }
        case SDL_MOUSEMOTION : {
            SDLWindow *win = windows[event->window.windowID];

            MotionEvent tr_event(event->motion.x, event->motion.y);
            tr_event.set_rel(event->motion.xrel, event->motion.yrel);
            tr_event.set_widget(win->widget);

            context->send_event(tr_event);
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            [[fallthrough]];
        }
        case SDL_MOUSEBUTTONUP: {
            SDLWindow *win = windows[event->window.windowID];
            if (win == nullptr) {
                printf("winid %d not found\n", event->window.windowID);
                break;
            }

            auto type = (event->type == SDL_MOUSEBUTTONDOWN) ? Event::MousePress : Event::MouseRelease;

            MouseEvent tr_event(type, event->button.x, event->button.y, event->button.clicks);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);

            context->send_event(tr_event);
            break;
        }
        case SDL_KEYDOWN: {
            [[fallthrough]];
        }
        case SDL_KEYUP: {
            SDLWindow *win = windows[event->window.windowID];
            auto type = (event->type == SDL_KEYDOWN) ? Event::KeyPress : Event::KeyRelease;

            auto keycode = SDLTranslateKey(event->key.keysym.sym);
            auto mod     = SDLTranslateMod(event->key.keysym.mod);

            KeyEvent tr_event(type, keycode, mod);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(time);
            break;
        }
        default: {
            //printf("unhandled event %d\n", event->type);
        }
    }
    return 0;
}
void SDLDriver::tr_window_event(SDL_Event *event) {
    // Get window from map
    SDLWindow *win = windows[event->window.windowID];

    switch(event->window.event) {
        case SDL_WINDOWEVENT_MOVED : {
            MoveEvent tr_event(event->window.data1, event->window.data2);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_RESIZED : {
            ResizeEvent tr_event(event->window.data1, event->window.data2);
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_EXPOSED : {
            PaintEvent tr_event;
            tr_event.set_widget(win->widget);
            tr_event.set_timestamp(GetTicks());
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_ENTER : {
            [[fallthrough]];
        }
        case SDL_WINDOWEVENT_LEAVE : {
            auto type = (event->window.event == SDL_WINDOWEVENT_ENTER) ? Event::MouseEnter : Event::MouseLeave;
            int gx, gy;
            int x, y;

            // Get current mouse position
            SDL_GetMouseState(&gx, &gy);
            SDL_GetWindowPosition(win->win, &x, &y);

            MotionEvent tr_event(gx - x, gy - y);
            tr_event.set_widget(win->widget);
            tr_event.set_type(type);
            context->send_event(tr_event);
            break;
        }
        case SDL_WINDOWEVENT_FOCUS_GAINED : {
            [[fallthrough]];
        }
        case SDL_WINDOWEVENT_FOCUS_LOST : {
            auto type = (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) ? Event::FocusGained : Event::FocusLost;
            WidgetEvent tr_event(type);
            tr_event.set_widget(win->widget);
            context->send_event(tr_event);
            break;
        }
        default : {
            break;
        }
    }
}

void SDLDriver::pump_events(UIContext *ctxt) {
    context = ctxt;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        sdl_event_filter(&event);
    }
}

window_t SDLDriver::window_create(const char_t * title, int width, int height,WindowFlags flags){
    // Translate flags to SDL flags
    uint32_t sdl_flags = SDL_WINDOW_ALLOW_HIGHDPI;
    if ((flags & WindowFlags::Resizable) == WindowFlags::Resizable) {
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    }

    SDL_Window *win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        sdl_flags
    );
    auto ret = new SDLWindow(win, this);
    window_add(ret);
    return ret;
}
void    SDLDriver::window_add(SDLWindow *win) {
    windows[win->id()] = win;
}
void    SDLDriver::window_del(SDLWindow *win) {
    windows.erase(win->id());

    // Maybe we should walk through the event queue and remove all events for this window?
}
u8string SDLDriver::clipboard_get() {
    char *str = SDL_GetClipboardText();
    u8string ret = u8string(str);
    SDL_free(str);
    return ret;
}
void SDLDriver::clipboard_set(const char_t *str) {
    SDL_SetClipboardText(str);
}
bool SDLDriver::timer_del(Object *obj, timerid_t id) {
    BTK_UNUSED(obj);

    auto ctxt = context;
    if (ctxt == nullptr) {
        ctxt = GetUIContext();
    }
    bool v = SDL_RemoveTimer(id) == SDL_TRUE;

    // Walk queue, drop timer if it's in there
    ctxt->walk_event([&](Event &ev){
        if (ev.type() == Event::Timer) {
            auto tev = static_cast<TimerEvent&>(ev);
            if (tev.timerid() == id) {
                return EventWalk::Drop;
            }
        }
        return EventWalk::Continue;
    });
    return v;
}
timerid_t SDLDriver::timer_add(Object *obj, uint32_t ms) {
    struct TimerData {
        UIContext *ctxt;
        Object *obj;
        timerid_t id;
    };
    auto data = new TimerData();
    data->obj = obj;
    data->ctxt = context;
    data->id = 0;

    if (data->ctxt == nullptr) {
        data->ctxt = GetUIContext();
    }
    BTK_ASSERT_MSG(data->ctxt, "No UI context");

    //Add sdl_timer
    auto cb = [](uint32_t ms,void *user) {
        auto up = static_cast<TimerData*>(user);
        auto ctxt = up->ctxt;
        auto obj = up->obj;
        auto id  = up->id;

        if (id != 0) {
            TimerEvent event(obj, id);
            event.set_timestamp(GetTicks());
            ctxt->send_event(event);
        }

        return ms;
    };
    auto id = SDL_AddTimer(ms, cb, data);
    if (id == 0) {
        delete data;
    }
    data->id = id;
    return id;
}
int SDLDriver::font() {
    if (def_font == -1) {
        auto finfo = fc_context.match("");
        auto face  = ft_manager.new_face(finfo.file.c_str(), finfo.index);

        if (!face.empty()) {
            face->set_dpi(xdpi, ydpi);
            def_font = ft_stash.add_font(face);
        }
    }
    return def_font;
}

// Window

SDLWindow::~SDLWindow() {
    // Remove self from driver
    driver->window_del(this);
    SDL_DestroyWindow(win);
}
Size SDLWindow::size() const {
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    return Size(w, h);
}
Point SDLWindow::position() const {
    int x, y;
    SDL_GetWindowPosition(win, &x, &y);
    return Point(x, y);
}

void SDLWindow::move(int x, int y) {
    SDL_SetWindowPosition(win, x, y);
}
void SDLWindow::resize(int w, int h) {
    SDL_SetWindowSize(win, w, h);
}
void SDLWindow::show(bool v) {
    if (v) {
        SDL_ShowWindow(win);
    }
    else {
        SDL_HideWindow(win);
    }
}
void SDLWindow::raise() {
    SDL_RaiseWindow(win);
}
void SDLWindow::set_title(const char_t *title) {
    SDL_SetWindowTitle(win, title);
}
void SDLWindow::set_icon(const PixBuffer &buf) {
    SDL_Surface *ref = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<void *>(buf.pixels()),
        buf.width(),
        buf.height(),
        32,
        buf.pitch(),
        buf.bpp() == 32 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24
    );
    SDL_SetWindowIcon(win, ref);
    SDL_FreeSurface(ref);
}
widget_t SDLWindow::bind_widget(widget_t widget) {
    widget_t ret = this->widget;
    this->widget = widget;
    return ret;
}
gcontext_t SDLWindow::gc_create() {
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        return nullptr;
    }
#if !defined(NDEBUG)
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    printf("Renderer: %s\n", info.name);
    printf("Texture formats: %d\n", info.num_texture_formats);
    for (int i = 0; i < info.num_texture_formats; i++) {
        printf("  %d: %s\n", i, SDL_GetPixelFormatName(info.texture_formats[i]));
    }
#endif
    return new SDLGraphicsContext(driver,win,renderer);
}
uint32_t SDLWindow::id() const {
    return SDL_GetWindowID(win);
}

// SDLGraphicsContext / SDLTexture
SDLGraphicsContext::~SDLGraphicsContext() {
    SDL_DestroyRenderer(renderer);
}
void SDLGraphicsContext::draw_image(const FRect &dst, texture_t t, const Rect &src) {
#if SDL_GC_FLOAT
    using SRect = SDL_FRect;
#else
    using SRect = SDL_Rect;
#endif
    SRect sdst;

    sdst.x = dst.x;
    sdst.y = dst.y;
    sdst.w = dst.w;
    sdst.h = dst.h;

#if SDL_GC_FLOAT
    SDL_RenderCopyF(renderer, static_cast<SDLTexture *>(t)->tex, reinterpret_cast<const SDL_Rect*>(&src), &sdst);
#else
    SDL_RenderCopy(renderer, static_cast<SDLTexture *>(t)->tex, reinterpret_cast<const SDL_Rect*>(&src), &sdst);
#endif
}
void SDLGraphicsContext::draw_points(const FPoint *fp, int n) {
#if SDL_GC_FLOAT
    SDL_RenderDrawPointsF(renderer, reinterpret_cast<const SDL_FPoint*>(fp), n);
#else
    for (int i = 0; i < n; i++) {
        SDL_RenderDrawPoint(renderer, fp[i].x, fp[i].y);
    }
#endif
}
void SDLGraphicsContext::draw_lines(const FPoint *fp, int n) {
    if (antialias) {
        return aa_draw_lines(fp, n);
    }
#if SDL_GC_FLOAT
    SDL_RenderDrawLinesF(renderer, reinterpret_cast<const SDL_FPoint*>(fp), n);
#else
    for (int i = 0; i < n - 1; i++) {
        SDL_RenderDrawLine(renderer, fp[i].x, fp[i].y, fp[i+1].x, fp[i+1].y);
    }
#endif
}
void SDLGraphicsContext::draw_rects(const FRect *fr, int n) {
#if SDL_GC_FLOAT
    SDL_RenderDrawRectsF(renderer, reinterpret_cast<const SDL_FRect*>(fr), n);
#else
    for (int i = 0; i < n; i++) {
        SDL_Rect r;
        r.x = fr[i].x;
        r.y = fr[i].y;
        r.w = fr[i].w;
        r.h = fr[i].h;
        SDL_RenderDrawRect(renderer, &r);
    }
#endif
}
void SDLGraphicsContext::fill_rects(const FRect *fr, int n) {
#if SDL_GC_FLOAT
    SDL_RenderFillRectsF(renderer, reinterpret_cast<const SDL_FRect*>(fr), n);
#else
    for (int i = 0; i < n; i++) {
        SDL_Rect r;
        r.x = fr[i].x;
        r.y = fr[i].y;
        r.w = fr[i].w;
        r.h = fr[i].h;
        SDL_RenderFillRect(renderer, &r);
    }
#endif
}

// Anti-aliasing

void SDLGraphicsContext::aa_draw_lines(const FPoint *fp, int count) {
    // Btk PixUtils raster
    // AAFn (x,y,gradient[0..1f])

    uint8_t r, g, b, a;
    float prev_a = 1.0;
    SDL_BlendMode mode;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    SDL_GetRenderDrawBlendMode(renderer, &mode);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    auto shade = [&, this](float x, float y, float alpha){
        if (prev_a != alpha) {
            prev_a = alpha;
            SDL_SetRenderDrawColor(renderer, r, g, b, a * alpha);
        }
        SDL_RenderDrawPoint(renderer, x, y);
    };
    auto ipart = [](float x){
        return int(x);
    };
    auto fpart = [&](float x){
        return x - ipart(x);
    };
    auto rfpart = [&](float x){
        return 1.0f - fpart(x);
    };

    for (int i = 0; i < count - 1; i++) {
        float x1 = fp[i].x;
        float y1 = fp[i].y;
        float x2 = fp[i+1].x;
        float y2 = fp[i+1].y;

        // Do Wu xiao-han algorithm
        bool steep = std::abs(y2 - y1) > std::abs(x2 - x1);
        if(steep){
            std::swap(x1,y1);
            std::swap(x2,y2);
        }
        if(x1 > x2){
            std::swap(x1,x2);
            std::swap(y1,y2);
        }

        float dx = x2 - x1;
        float dy = y2 - y1;
        float gradient;
        if(dx == 0.0){
            // Vertical line
            gradient = 1.0;
        }
        else{
            gradient = dy / dx;
        }

        // Start and end points
        float xend = std::round(x1);
        float yend = y1 + gradient * (xend - x1);
        float xgap = rfpart(x1 + 0.5f);
        float xpxl1 = xend;
        float ypxl1 = ipart(yend);
        if(steep){
            shade(ypxl1    ,xpxl1,rfpart(yend) * xgap);
            shade(ypxl1 + 1,xpxl1, fpart(yend) * xgap);
        }
        else{
            shade(xpxl1,ypxl1    ,rfpart(yend) * xgap);
            shade(xpxl1,ypxl1 + 1, fpart(yend) * xgap);
        }
        float intery = yend + gradient;

        //Second point
        xend = std::round(x2);
        yend = y2 + gradient * (xend - x2);
        xgap = rfpart(x2 + 0.5f);
        float xpxl2 = xend;
        float ypxl2 = ipart(yend);
        if(steep){
            shade(ypxl2    ,xpxl2,rfpart(yend) * xgap);
            shade(ypxl2 + 1,xpxl2, fpart(yend) * xgap);
        }
        else{
            shade(xpxl2,ypxl2    ,rfpart(yend) * xgap);
            shade(xpxl2,ypxl2 + 1, fpart(yend) * xgap);
        }

        // Draw the middle part
        for(int x = ipart(xpxl1) + 1;x <= ipart(xpxl2) - 1;x++){
            if(steep){
                shade(ipart(intery)    ,x,rfpart(intery));
                shade(ipart(intery) + 1,x, fpart(intery));
            }
            else{
                shade(x,ipart(intery)    ,rfpart(intery));
                shade(x,ipart(intery) + 1, fpart(intery));
            }
            intery += gradient;
        }

    }

    // Done , restore blend mode
    SDL_SetRenderDrawBlendMode(renderer, mode);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
texture_t SDLGraphicsContext::texture_create(int w, int h, PixFormat fmt) {
    SDL_Texture *tex = SDL_CreateTexture(
        renderer,
        fmt == PixFormat::RGB24 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET,
        w,
        h
    );
    if (tex == nullptr) {
        return nullptr;
    }
    return new SDLTexture(tex, this);
}
void SDLGraphicsContext::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
void SDLGraphicsContext::begin() {
    int rw, rh;
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetRendererOutputSize(renderer ,&rw, &rh);

    x_scale = float(rw) / w;
    y_scale = float(rh) / h;

    SDL_RenderSetScale(
        renderer,
        x_scale,
        y_scale
    );

    // Clear state
    SDL_RenderSetClipRect(renderer, nullptr);
    SDL_RenderSetViewport(renderer, nullptr);
}
void SDLGraphicsContext::end() {
    SDL_RenderPresent(renderer);
}
void SDLGraphicsContext::clear() {
    SDL_RenderClear(renderer);
}
bool SDLGraphicsContext::set_antialias(bool v) {
    antialias = v;
    return false;
}
bool SDLGraphicsContext::has_feature(int f) const {
    switch (f) {
        case AntiAliasing : 
            return true;
        case Blending : 
            return true;
        case Scissor :
            return true;
        case TextureLimit :
            return true;
        default:
            return false;
    }
}
bool SDLGraphicsContext::set_feature(int f, ...) {
    bool ret;
    va_list varg;
    va_start(varg, f);

    switch (f) {
        case AntiAliasing : {
            bool v = va_arg(varg, varg_bool_t);
            ret = set_antialias(v);
            break;
        }
        case Blending : {
            SDL_BlendMode mode;
            BlendMode v = va_arg(varg, BlendMode);
            if (v == BlendMode::Alpha) {
                mode = SDL_BLENDMODE_BLEND;
            }
            else if (v == BlendMode::Add) {
                mode = SDL_BLENDMODE_ADD;
            }
            else if (v == BlendMode::Modulate) {
                mode = SDL_BLENDMODE_MOD;
            }
            else {
                ret = false;
                break;
            }
            ret = SDL_SetRenderDrawBlendMode(renderer, mode) == 0;
            break;
        }
        default : {
            ret = false;
            break;
        }
    }


    va_end(varg);

    return ret;
}
bool SDLGraphicsContext::get_info(int f, ...) {
    bool ret;
    va_list varg;
    va_start(varg, f);

    switch (f) {
        case TextureLimit : {
            Size *psize = va_arg(varg, Size*);
            SDL_RendererInfo info;
            ret = SDL_GetRendererInfo(renderer, &info) == 0;
            if (ret) {
                psize->w = info.max_texture_width;
                psize->h = info.max_texture_height;
            }

            break;
        }
        default : {
            ret = false;
            break;
        }
    }

    va_end(varg);
    return ret;
}
// Draw text
void SDLGraphicsContext::draw_text(font_t f, align_t align, u8string_view txt, float x, float y) {
    float size;
    int id = -1;
    if (f) {
        id   = static_cast<SDLFont*>(f)->font;
        size = static_cast<SDLFont*>(f)->fsize;
    }
    else {
        id   = driver->font();
        size = 12; 
    }
    int talign = 0;

    // Translate align to fons align
    if ((align & Alignment::Bottom) == Alignment::Bottom) {
        talign |= FONS_ALIGN_BOTTOM;
    }
    if ((align & Alignment::Top) == Alignment::Top) {
        talign |= FONS_ALIGN_TOP;
    }
    if ((align & Alignment::Baseline) == Alignment::Baseline) {
        talign |= FONS_ALIGN_BASELINE;
    }
    if ((align & Alignment::Right) == Alignment::Right) {
        talign |= FONS_ALIGN_RIGHT;
    }
    if ((align & Alignment::Left) == Alignment::Left) {
        talign |= FONS_ALIGN_LEFT;
    }
    if ((align & Alignment::Middle) == Alignment::Middle) {
        talign |= FONS_ALIGN_MIDDLE;
    }
    if ((align & Alignment::Center) == Alignment::Center) {
        talign |= FONS_ALIGN_CENTER;
    }

    uint8_t r, g, b, a;
    SDL_BlendMode mode;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    SDL_GetRenderDrawBlendMode(renderer, &mode);

    // To RRGGBBAA uint32
    uint32_t pixel = 0;
    pixel |= r << 24;
    pixel |= g << 16;
    pixel |= b << 8;
    pixel |= a;

    txt_render.set_align(talign);
    txt_render.set_color(pixel);
    txt_render.set_size(size);
    txt_render.set_font(id);
    txt_render.draw_text(x, y, txt.data(), txt.data() + txt.size());
    txt_render.flush();

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_SetRenderDrawBlendMode(renderer, mode);
}


// Texture

SDLTexture::~SDLTexture() {
    SDL_DestroyTexture(tex);
}
Size SDLTexture::size() const {
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    return Size(w, h);
}
void SDLTexture::update(cpointer_t pix, int x, int y, int w, int h, int stride) {
    SDL_Rect rect = {x, y, w, h};
    SDL_UpdateTexture(tex, &rect, pix, stride);
}
any_t SDLTexture::context() const {
    return ctxt;
}

// Font
SDLFont::SDLFont(int f) : font(f) {
    fsize = 12;
}
SDLFont::~SDLFont() {

}

u8string_view SDLFont::name() const {
    return {};
}
int SDLFont::size() const {
    return fsize;
}
bool SDLFont::set_size(int size) {
    fsize = size;
    return true;
}
SDLFont *SDLFont::clone() const  {
    auto f = new SDLFont(font);
    f->fsize = fsize;
    return f;
}

#undef SDL_CAST

BTK_NS_END2()

BTK_NS_BEGIN

GraphicsDriverInfo SDLDriverInfo = {
    []() -> GraphicsDriver * {
        return new SDLDriver();
    },
    "SDL2",
};

BTK_NS_END