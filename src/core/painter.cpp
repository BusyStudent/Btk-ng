#include "build.hpp"
#include "common/utils.hpp"
#include "common/painterpath.hpp"
#include "common/pod_container.hpp"
#include "common/device_resource.hpp"

#if defined(_WIN32)
    #include "common/win32/wincodec.hpp"
    #undef min
    #undef max
#endif

#include <Btk/detail/platform.hpp>
#include <Btk/detail/reference.hpp>
#include <Btk/detail/device.hpp>
#include <Btk/detail/types.hpp>
#include <Btk/painter.hpp>
#include <Btk/font.hpp>
#include <algorithm>
#include <variant>
#include <stack>
#include <map>

// Helper macro for Make Pen, Brush Path, interface
#define BTK_RESOURCE_INTERFACE_IMPL(type) \
    COW_BASIC_IMPL(type)                                                          \
    void type::begin_mut() {                                                      \
        COW_MUT(priv);                                                            \
        priv->reset_manager();                                                    \
    }                                                                             \
    auto type::bind_device_resource(void *key, PaintResource *res) const -> void { \
        if (priv) return priv->add_resource(key, res);                             \
        Ref<PaintResource> g(res);                                                 \
        return;                                                                   \
    }                                                                             \
    auto type::query_device_resource(void *key) const -> PaintResource        * { \
        if (priv) return priv->get_resource(key);                                 \
        return nullptr;                                                           \
    }
    

BTK_NS_BEGIN

class PainterPathImpl : public PaintResourceManager, public Refable<PainterPathImpl> {
    public:
        enum : int {
            // ArcTo,
            MoveTo, 
            LineTo, 
            // QuadTo, 
            BezierTo, 
            ClosePath,
            SetWinding
        };

        // Opcode : Datalen
        // MoveTo   x , y      3 float
        // LineTo   x , y      3 float
        // QuadTo   xy, xy     5 float
        // BezierTo xy, xy, xy 7 float
        // ArcTo    xy, xy, r  6 float
        // ClosePath  [Empty]  1 float

        std::vector<float> paths; //< Path Opcode, Args .... 
        FMatrix            matrix = FMatrix::Identity();
        float              last_x = 0;
        float              last_y = 0;

        void add_cmds(int cmd, std::initializer_list<float> args = {}) {
            paths.emplace_back(cmd);
            for (auto v : args) {
                paths.emplace_back(v);
            }
        }
};

class BrushImpl       : public PaintResourceManager, public Refable<BrushImpl> {
    public:
        CoordinateMode cmode  = CoordinateMode::Relative;
        BrushType      btype  = BrushType::Solid;
        FMatrix        matrix = FMatrix::Identity();
        FRect          rect   = {0.0f, 0.0f, 1.0f, 1.0f};

        std::variant<
            GLColor, 
            PixBuffer, 
            LinearGradient,
            RadialGradient,
            Ref<PaintResource>  //< For textures
        >              data = GLColor(0.0f, 0.0f, 0.0f, 1.0f);
};
class PenImpl          : public PaintResourceManager, public Refable<PenImpl> {
    public:
        DashStyle                dstyle = DashStyle::Solid; //< Default no dash style
        LineJoin                 join = LineJoin::Miter;
        LineCap                  cap  = LineCap::Flat;
        std::vector<float>       dashes;
        float                    dash_offset = 0.0f;
        float                    miter_limit = 10.0f;
};
class TextureImpl       : public Trackable {
    public:
        Ref<AbstractTexture>     texture;

        void on_texture_destroyed() { texture.reset(); }
};


class PainterState {
    public:
        Brush          brush  = { Color::Black };          //< Current fill brush
        Font           font   = { };          //< Current text font
        Pen            pen    = { };          //< Current fill pen

        float          width  = 1.0f;         //< Current stroke width
        float          alpha  = 1.0f;         //< Current Paint alpha
        bool           antialias = true;      //< Whether antialias is enabled

        bool           has_scissor = false;   //< Does has the scissor
        FRect          scissor     = { };     //< The current scissor
        FMatrix        scissor_mat = FMatrix::Identity(); //< The transform matrix of scissor

        FMatrix        matrix = FMatrix::Identity(); //< Current transform matrix

        Alignment      alignment = AlignLeft | AlignTop; //< Alignment of the text
};
class PainterImpl {
    public:
        PainterImpl(PaintDevice *dev, bool owned);
        ~PainterImpl();

        void begin();
        void end();
        void clear();

        void save();
        void reset();
        void restore();

        // Dirty state 
        void check_dirty();
        void mark_all_dirty();
        void mark_dirty_from(const PainterState &a, const PainterState &b);

        std::unique_ptr<PaintDevice> device = { }; //< PaintDevice
        Ref<PaintContext>            ctxt  = { }; //< Painter Context
        std::stack<PainterState>     state = { }; //< Painter State
        PainterState              defstate = { }; //< Default State (used for compare)

        bool                         device_owned; //< Does the painter take ownership of this device

        struct {
            uint8_t antialias : 1; //< Antialias was changed by user
            uint8_t transform : 1; //< Transform matrix was changed by user
            uint8_t scissor   : 1; //< Scissor Rect was changed by user
            uint8_t alpha     : 1; //< alpha was changed by user
            uint8_t brush     : 1; //< brush was changed by user
            uint8_t pen       : 1; //< pen was changed by user
            uint8_t width     : 1; //< stroke width was changed by user
        } dirty;
};

inline PainterImpl::PainterImpl(PaintDevice *dev, bool owned) : 
    device(dev), 
    ctxt(dev->paint_context()),
    device_owned(owned) 
{
    // Push a default state 
    state.emplace(defstate);
    Btk_memset(&dirty, 0, sizeof(dirty));
}
inline PainterImpl::~PainterImpl() {
    if (!device_owned) {
        // Release ownership
        device.release();
    }
}

inline void PainterImpl::begin() {
    ctxt->begin();

    // Clear previous & Create the state object
    if (state.size() != 1) {
        state.pop();
    }
    // Sync state
    mark_all_dirty();
}
inline void PainterImpl::end() {
    ctxt->end();
    ctxt->swap_buffers();
}
inline void PainterImpl::clear() {
    ctxt->clear(state.top().brush);
}
inline void PainterImpl::save() {
    // Save current state, We didnot need mark dirty
    state.emplace(state.top());
}
inline void PainterImpl::reset() {
    // Reset to default state, need compare state to set dirty state
    mark_dirty_from(state.top(), defstate);
    state.top() = defstate;
}
inline void PainterImpl::restore() {
    BTK_ASSERT(state.size() > 1); // At least has one
    
    // Get prev state
    auto prev = state.top();
    state.pop();

    mark_dirty_from(state.top(), prev);
}

inline void PainterImpl::check_dirty() {
    // Keep track of dirty state
    if (dirty.transform) {
        dirty.transform = false;
        ctxt->set_transform(&state.top().matrix);
    }
    if (dirty.scissor) {
        if (!state.top().has_scissor) {
            ctxt->set_scissor(nullptr);
        }
        else {
            PaintScissor scissor;
            scissor.rect = state.top().scissor;
            scissor.matrix = state.top().scissor_mat;

            ctxt->set_scissor(&scissor);
        }
        dirty.scissor = false;
    }
    if (dirty.brush) {
        dirty.brush = false;
        ctxt->set_brush(state.top().brush);
    }
    if (dirty.pen) {
        dirty.pen = false;
        ctxt->set_pen(state.top().pen);
    }
    if (dirty.width) {
        dirty.width = false;
        ctxt->set_stroke_width(state.top().width);
    }
    if (dirty.alpha) {
        dirty.alpha = false;
        ctxt->set_alpha(state.top().alpha);
    }
    if (dirty.antialias) {
        dirty.antialias = false;
        ctxt->set_antialias(state.top().antialias);
    }
}
inline void PainterImpl::mark_dirty_from(const PainterState &self, const PainterState &other) {
    // Transform
    if (self.matrix != other.matrix) {
        dirty.transform = true;
    }

    // Scissor
    if (self.has_scissor != other.has_scissor) {
        dirty.scissor = true;
    }
    if (self.has_scissor) {
        // Has sicssor, compare the rectangle
        dirty.scissor = (self.scissor == other.scissor) && (self.scissor_mat == other.scissor_mat);
    }

    // Alpha
    if (self.alpha != other.alpha) {
        dirty.alpha = true;
    }

    // antialias
    if (self.antialias != other.antialias) {
        dirty.antialias = true;
    }

    // Pen
    if (self.pen != other.pen) {
        dirty.pen = true;
    }

    // Brush
    if (self.brush != other.brush) {
        dirty.brush = true;
    }

    if (self.width != other.width) {
        dirty.width = true;
    }
}
inline void PainterImpl::mark_all_dirty() {
    constexpr uint8_t fill = ~uint8_t(0);
    for (int i = 0;i < sizeof(dirty); i++) {
        reinterpret_cast<uint8_t*>(&dirty)[i] = fill;
    }
}

Painter::Painter() {
    priv = nullptr;
}
Painter::Painter(PaintDevice *device, bool owned) {
    priv = new PainterImpl(device, owned);
    if (!priv->ctxt) {
        // No context provided
        delete priv;
        priv = nullptr;
    }
}
Painter::Painter(PixBuffer &buffer) : Painter(CreatePaintDevice(&buffer), true) { }
Painter::Painter(Texture   &texture) : Painter(texture.priv->texture.get()) { }

Painter::Painter(Painter &&p) {
    priv = p.priv;
    p.priv = nullptr;
}
Painter::~Painter() {
    delete priv;
}
void Painter::draw_rect(float x, float y, float w, float h) {
    priv->check_dirty();
    priv->ctxt->draw_rect(x, y, w, h);
}
void Painter::draw_line(float x1, float y1, float x2, float y2) {
    priv->check_dirty();
    priv->ctxt->draw_line(x1, y1, x2, y2);
}
void Painter::draw_circle(float x, float y, float r) {
    priv->check_dirty();
    priv->ctxt->draw_ellipse(x, y, r, r);
}
void Painter::draw_ellipse(float x, float y, float xr, float yr) {
    priv->check_dirty();
    priv->ctxt->draw_ellipse(x, y, xr, yr);
}
void Painter::draw_rounded_rect(float x, float y, float w, float h, float r) {
    priv->check_dirty();
    priv->ctxt->draw_rounded_rect(x, y, w, h, r);
}
void Painter::draw_image(const Texture &tex, const FRect *dst, const FRect *src) {
    if (tex.empty()) {
        return;
    }
    priv->check_dirty();
    priv->ctxt->draw_image(tex.priv->texture.get(), dst, src);
}
void Painter::draw_text(u8string_view txt, float x, float y) {
    auto &state = priv->state.top();

    priv->check_dirty();
    priv->ctxt->draw_text(state.alignment, state.font, txt, x, y);
}
void Painter::draw_text(const TextLayout &lay, float x, float y) {
    if (lay.priv == nullptr) {
        return;
    }
    auto &state = priv->state.top();

    priv->check_dirty();
    priv->ctxt->draw_text(state.alignment, lay, x, y);
}
void Painter::draw_path(const PainterPath &path) {
    if (path.priv) {
        priv->check_dirty();
        priv->ctxt->draw_path(path);
    }
}

void Painter::fill_rect(float x, float y, float w, float h) {
    priv->check_dirty();
    priv->ctxt->fill_rect(x, y, w, h);
}
void Painter::fill_circle(float x, float y, float r) {
    priv->check_dirty();
    priv->ctxt->fill_ellipse(x, y, r, r);
}
void Painter::fill_ellipse(float x, float y, float xr, float yr) {
    priv->check_dirty();
    priv->ctxt->fill_ellipse(x, y, xr, yr);
}
void Painter::fill_rounded_rect(float x, float y, float w, float h, float r) {
    priv->check_dirty();
    priv->ctxt->fill_rounded_rect(x, y, w, h, r);
}
void Painter::fill_path(const PainterPath &path) {
    if (path.priv) {
        priv->check_dirty();
        priv->ctxt->fill_path(path);
    }
}
void Painter::fill_mask(const Texture &tex, const FRect *dst, const FRect *src) {
    if (tex.empty()) {
        return;
    }
    priv->check_dirty();
    priv->ctxt->fill_mask(tex.priv->texture.get(), dst, src);
}

// Begin
void Painter::begin() {
    priv->begin();
}
void Painter::end() {
    priv->end();
}
void Painter::clear() {
    priv->clear();
}

// State
void Painter::save() {
    priv->save();
}
void Painter::reset() {
    priv->reset();
}
void Painter::restore() {
    priv->restore();
}

// Set State
void Painter::set_text_align(Alignment alignment) {
    priv->state.top().alignment = alignment;
}
void Painter::set_colorf(float r, float g, float b, float a) {
    priv->state.top().brush.set_color(GLColor(r, g, b, a));
    priv->dirty.brush = true;
}
void Painter::set_color(uint8_t r, uint8_t g , uint8_t b, uint8_t a) {
    priv->state.top().brush.set_color(Color(r, g, b, a));
    priv->dirty.brush = true;
}
void Painter::set_brush(const Brush &b) {
    priv->state.top().brush = b;
    priv->dirty.brush = true;
}
void Painter::set_font(const Font &font) {
    priv->state.top().font = font;
}
void Painter::set_pen(const Pen &p) {
    priv->state.top().pen = p;
    priv->dirty.pen = true;
}
void Painter::set_stroke_width(float w) {
    priv->state.top().width = w;
    priv->dirty.width = true;
}
void Painter::set_alpha(float alpha) {
    priv->state.top().alpha = alpha;
    priv->dirty.alpha = true; //< Mark dirty
}
void Painter::set_antialias(bool v) {
    priv->state.top().antialias = v;
    priv->dirty.antialias = true;
}

// Get
auto Painter::alpha() const -> float {
    return priv->state.top().alpha;
}
auto Painter::context() const -> PaintContext * {
    return priv->ctxt.get();
}

// Transform
void Painter::transform(const FMatrix &mat) {
    priv->state.top().matrix = priv->state.top().matrix * mat;
    priv->dirty.transform = true;
}
void Painter::translate(float x, float y) {
    priv->state.top().matrix.translate(x, y);
    priv->dirty.transform = true;
}
void Painter::scale(float x, float y) {
    priv->state.top().matrix.scale(x, y);
    priv->dirty.transform = true;
}
void Painter::skew_x(float x) {
    priv->state.top().matrix.skew_x(x);
    priv->dirty.transform = true;
}
void Painter::skew_y(float y) {
    priv->state.top().matrix.skew_y(y);
    priv->dirty.transform = true;
}
void Painter::rotate(float a) {
    priv->state.top().matrix.rotate(a);
    priv->dirty.transform = true;
}
void Painter::reset_transform() {
    priv->state.top().matrix = FMatrix::Identity();
    priv->dirty.transform = true;
}

// Scissor
void Painter::set_scissor(float x, float y, float w, float h) {
    auto &state = priv->state.top();

    // Record scissor info
    state.scissor = FRect(x, y, w, h);
    state.scissor_mat = state.matrix;
    
    state.has_scissor = true;

    priv->dirty.scissor = true;
}
void Painter::scissor(float x, float y, float w, float h) {
    auto &state = priv->state.top();
    if (!state.has_scissor) {
        return set_scissor(x, y, w, h);
    }
    BTK_ONCE(BTK_LOG("Painter intersect scissor is experiemental\n"));

    // TODO : We need to transform prev into target space
    auto scissor_mat = state.scissor_mat;
    auto invmatrix = state.matrix;
    invmatrix.invert();

    scissor_mat *= invmatrix;

    // Get transformed rect
    auto start = FPoint(state.scissor.x, state.scissor.y) * scissor_mat;
    auto end = FPoint(state.scissor.x + state.scissor.w, state.scissor.y + state.scissor.h) * scissor_mat;

    FRect rect(
        start.x, start.y,
        end.x - start.x, end.y - start.y
    );
    rect = rect.intersected(FRect(x, y, w, h));

    state.scissor = rect;
    state.scissor_mat = state.matrix;

    // state.scissor = state.scissor.intersected(FRect(x, y, w, h));
    // state.scissor_mat = state.matrix;

    state.has_scissor = true;
    priv->dirty.scissor = true;
}
void Painter::reset_scissor() {
    priv->state.top().has_scissor = false;
    priv->dirty.scissor = true;
}

// Texture creation functions
auto Painter::create_texture(PixFormat fmt, int w, int h, float xdpi, float ydpi) -> Texture {
    auto tex = priv->ctxt->create_texture(fmt, w, h, xdpi, ydpi);
    if (!tex) {
        return { };
    }
    Texture t;
    t.priv = new TextureImpl;
    t.priv->texture = tex;

    tex->signal_destroyed().connect(
        &TextureImpl::on_texture_destroyed, t.priv
    );
    return t;
}
auto Painter::create_texture(const PixBuffer &buf) -> Texture {
    Texture t = create_texture(buf.format(), buf.width(), buf.height());
    t.update(
        nullptr,
        buf.pixels(),
        buf.pitch()
    );
    return t;
}

// Interface for painter on window
void Painter::notify_dpi_changed(float xdpi, float ydpi) {

#if !defined(BTK_NO_RTTI)
    if (!dynamic_cast<WindowDevice*>(priv->device.get())) {
        // Not supported
        return;
    }
#endif

    static_cast<WindowDevice*>(priv->device.get())->set_dpi(xdpi, ydpi);
}
void Painter::notify_resize(int w, int h) {

#if !defined(BTK_NO_RTTI)
    if (!dynamic_cast<WindowDevice*>(priv->device.get())) {
        // Not supported
        return;
    }
#endif

    static_cast<WindowDevice*>(priv->device.get())->resize(w, h);
}

// Move
void Painter::operator =(Painter && p) {
    delete priv;
    priv = p.priv;
    p.priv = nullptr;
}
void    Painter::Init() {

#if defined(_WIN32)
    Win32::Wincodec::Init();
#endif

    Font::Init();
}
void    Painter::Shutdown() {

#if defined(_WIN32)
    Win32::Wincodec::Shutdown();
#endif

    Font::Shutdown();
}

// Leagcay create
Painter Painter::FromWindow(AbstractWindow *window) {
    auto device = CreatePaintDevice(window);
    if (!device) {
        return {};
    }
    return Painter(device, true);
}
Painter Painter::FromPixBuffer(PixBuffer &buffer) {
    auto device = CreatePaintDevice(&buffer);
    if (!device) {
        return {};
    }
    return Painter(device, true);
}

// Impl Brush
BTK_RESOURCE_INTERFACE_IMPL(PainterPath);
BTK_RESOURCE_INTERFACE_IMPL(Brush);
BTK_RESOURCE_INTERFACE_IMPL(Pen);


// Impl path
PainterPath::PainterPath() {
    priv = nullptr;
}
void PainterPath::open() {
    // Begin motify
    begin_mut();
}
void PainterPath::close() {
    // No-op
}
void PainterPath::move_to(float x, float y) {
    priv->add_cmds(PainterPathImpl::MoveTo, {x, y});
    priv->last_x = x;
    priv->last_y = y;
}
void PainterPath::line_to(float x, float y) {
    priv->add_cmds(PainterPathImpl::LineTo, {x, y});
    priv->last_x = x;
    priv->last_y = y;
}
void PainterPath::quad_to(float x1, float y1, float x2, float y2) {
    fallback_painter_quad_to(
        this, 
        priv->last_x,
        priv->last_y,
        x1, y1,
        x2, y2
    );
}
void PainterPath::bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) {
    priv->add_cmds(PainterPathImpl::BezierTo, {x1, y1, x2, y2, x3, y3});
    priv->last_x = x3;
    priv->last_y = y3;
}
void PainterPath::arc_to(float x1, float y1, float x2, float y2, float radius) {
    fallback_painter_arc_to(
        this, 
        priv->last_x,
        priv->last_y,
        x1, y1,
        x2, y2,
        radius
    );
}
void PainterPath::close_path() {
    priv->add_cmds(PainterPathImpl::ClosePath);
}
void PainterPath::clear() {
    // Reuse memory
    if (priv) {
        priv->paths.clear();
    }
}
void PainterPath::set_transform(const FMatrix &mat) {
    begin_mut();
    priv->matrix = mat;
}
void PainterPath::set_winding(PathWinding winding) {
    priv->add_cmds(PainterPathImpl::SetWinding, { float(winding) });
}

void PainterPath::stream(PainterPathSink *sink) const {
    BTK_ASSERT(sink != nullptr);
    if (empty()) {
        return;
    }
    sink->open();

    auto &paths = priv->paths;
    for (auto iter = paths.data(); iter != paths.data() + paths.size(); ) {
        switch (int(*iter)) {
            case PainterPathImpl::MoveTo : {
                sink->move_to(iter[1], iter[2]);
                iter += 3;
                break;
            }
            case PainterPathImpl::LineTo : {
                sink->line_to(iter[1], iter[2]);
                iter += 3;
                break;
            }
            // case PainterPathImpl::QuadTo : {
            //     sink->quad_to(iter[1], iter[2], iter[3], iter[4]);
            //     iter += 5;
            //     break;
            // }
            case PainterPathImpl::BezierTo : {
                sink->bezier_to(iter[1], iter[2], iter[3], iter[4], iter[5], iter[6]);
                iter += 7;
                break;
            }
            // case PainterPathImpl::ArcTo : {
            //     sink->arc_to(iter[1], iter[2], iter[3], iter[4], iter[5]);
            //     iter += 6;
            //     break;
            // }
            case PainterPathImpl::ClosePath : {
                sink->close_path();
                iter += 1;
                break;
            }
            case PainterPathImpl::SetWinding : {
                sink->set_winding(PathWinding(iter[1]));
                iter += 2;
                break;
            }
        }
    }

    sink->close();
}
bool PainterPath::empty() const {
    if (priv) {
        return priv->paths.empty();
    }
    return true;
}
FRect PainterPath::bounding_box() const {
    if (empty()) {
        return FRect(0.0f, 0.0f, 0.0f, 0.0f);
    }
    // Calc the bounding box by curve
    float left = std::numeric_limits<float>::max();
    float top = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::min();
    float bottom = std::numeric_limits<float>::min();

    // Calculate the bounding box by command's control point
    for (auto iter = priv->paths.data(); iter != priv->paths.data() + priv->paths.size(); ) {
        switch (int(*iter)) {
            case PainterPathImpl::MoveTo :
            case PainterPathImpl::LineTo : {
                left = std::min(left, iter[1]);
                top = std::min(top, iter[2]);
                right = std::max(right, iter[1]);
                bottom = std::max(bottom, iter[2]);
                iter += 3;
                break;
            }
            case PainterPathImpl::BezierTo : {
                left = std::min({left, iter[1], iter[3], iter[5]});
                top = std::min({top, iter[2], iter[4], iter[6]});
                right = std::max({right, iter[1], iter[3], iter[5]});
                bottom = std::max({bottom, iter[2], iter[4], iter[6]});
                iter += 7;
                break;
            }
            case PainterPathImpl::ClosePath : {
                iter += 1;
                break;
            }
            case PainterPathImpl::SetWinding : {
                iter += 2;
                break;
            }
        }
    }

    // Update the width and height of the bounding box
    FRect box(left, top, right - left, bottom - top);

    return box;
}
bool  PainterPath::contains(float x, float y) const {
    BTK_ONCE(BTK_LOG("PainterPath::contains() unimplmented\n"));
    return false;
}

// Impl Brush
Brush::Brush() {
    priv = nullptr;
}
void Brush::set_color(const GLColor &c) {
    begin_mut();
    priv->data = c;
    priv->btype = BrushType::Solid;
}
void Brush::set_image(const PixBuffer &img) {
    begin_mut();
    priv->data = img;
    priv->btype = BrushType::Bitmap;
}
void Brush::set_gradient(const LinearGradient & g) {
    begin_mut();
    priv->data = g;
    priv->btype = BrushType::LinearGradient;
}
void Brush::set_gradient(const RadialGradient & g) {
    begin_mut();
    priv->data = g;
    priv->btype = BrushType::RadialGradient;
}
FRect     Brush::rect() const {
    if (priv) {
        return priv->rect;
    }
    return FRect(0.0f, 0.0f, 1.0f, 1.0f);
}
BrushType Brush::type() const {
    if (priv) {
        return priv->btype;
    }
    return BrushType::Solid;
}
GLColor   Brush::color() const {
    if (priv) {
        if (priv->btype == BrushType::Solid) {
            return std::get<GLColor>(priv->data);
        }
    }
    return Color::Black;
}
PixBuffer  Brush::bitmap() const {
    if (priv) {
        if (priv->btype == BrushType::Bitmap) {
            return std::get<PixBuffer>(priv->data);
        }
    }
    ::abort();
}
FMatrix     Brush::matrix() const {
    if (priv) {
        return priv->matrix;
    }
    ::abort();
}
CoordinateMode Brush::coordinate_mode() const {
    if (priv) {
        return priv->cmode;
    }
    return CoordinateMode::Relative;
}
const LinearGradient &Brush::linear_gradient() const {
    if (priv) {
        if (priv->btype == BrushType::LinearGradient) {
            return std::get<LinearGradient>(priv->data);
        }
    }
    // No data
    ::abort();
}
const RadialGradient &Brush::radial_gradient() const {
    if (priv) {
        if (priv->btype == BrushType::RadialGradient) {
            return std::get<RadialGradient>(priv->data);
        }
    }
    ::abort();
}

// Brush Helper
FPoint Brush::point_to_abs(PaintDevice *device, const FRect &obj, const FPoint &fp) const {
    FRect rel; //< Rectangle for rel convert
    switch (priv->cmode) {
        case CoordinateMode::Absolute : { return fp; }
        case CoordinateMode::Device : {
            auto [w, h] = device->size();
            rel.x = 0;
            rel.y = 0;
            rel.w = w;
            rel.h = h;
            break;
        }
        case CoordinateMode::Relative : {
            // On Drawing object
            rel = obj;
            break;
        }
    }
    FPoint ret;
    ret.x = rel.x + rel.w * fp.x;
    ret.y = rel.y + rel.h * fp.y;
    return ret;
}
FRect  Brush::rect_to_abs(PaintDevice *device, const FRect &obj, const FRect &fr) const {
    FRect rel; //< Rectangle for rel convert
    switch (priv->cmode) {
        case CoordinateMode::Absolute : { return fr; }
        case CoordinateMode::Device : {
            auto [w, h] = device->size();
            rel.x = 0;
            rel.y = 0;
            rel.w = w;
            rel.h = h;
            break;
        }
        case CoordinateMode::Relative : {
            // On Drawing object
            rel = obj;
            break;
        }
    }
    FRect ret;
    ret.x = rel.x + rel.w * fr.x;
    ret.y = rel.y + rel.h * fr.y;
    ret.w = rel.w * fr.w;
    ret.h = rel.h * fr.h;
    return ret;
}
bool  Brush::operator ==(const Brush &other) const {
    if (priv == other.priv) {
        return true;
    }
    return false;
}
bool  Brush::operator !=(const Brush &other) const {
    return !operator ==(other);
}

// Impl textures
Texture::Texture() {
    priv = nullptr;
}
Texture::Texture(Texture &&t) {
    priv = t.priv;
    t.priv = nullptr;
}
Texture::~Texture() {
    delete priv;
}
bool Texture::empty() const {
    if (priv) {
        // Internal data already destroyed
        return !priv->texture;
    }
    // No texture
    return true;
}
void Texture::clear() {
    delete priv;
    priv = nullptr;
}
void Texture::update(const Rect *where, cpointer_t data, uint32_t pitch) {
    if (!empty()) {
        priv->texture->update(where, data, pitch);
    }
}
FSize Texture::size() const {
    // MinGW Crashed on bitmap->GetSize()
    // auto [w, h] = priv->bitmap->GetPixelSize();
    if (empty()) {
        return FSize(0, 0);
    }

    return priv->texture->size();
}
Size  Texture::pixel_size() const {
    if (empty()) {
        return Size(0, 0);
    }

    return priv->texture->pixel_size();
}
FPoint Texture::dpi() const {
    if (empty()) {
        return FPoint(96.0f, 96.0f);
    }
    
    return priv->texture->dpi();
}
void   Texture::set_interpolation_mode(InterpolationMode mode) {
    if (priv) {
        priv->texture->set_interpolation_mode(mode);
    }
}

Texture &Texture::operator =(Texture &&t) {
    delete priv;
    priv = t.priv;
    t.priv = nullptr;
    return *this;
}
Texture &Texture::operator =(std::nullptr_t) {
    clear();
    return *this;
}

// Impl pen
Pen::Pen() {
    priv = nullptr;
}
void Pen::set_dash_pattern(const float *pat, size_t n) {
    begin_mut();
    priv->dstyle = DashStyle::Custom;
    priv->dashes.assign(pat, pat + n);
}
void Pen::set_dash_offset(float offset) {
    begin_mut();
    priv->dash_offset = offset;
}
void Pen::set_dash_style(DashStyle style) {
    begin_mut();
    priv->dstyle = style;
}
void Pen::set_miter_limit(float limit) {
    begin_mut();
    priv->miter_limit = limit;
}
void Pen::set_line_join(LineJoin join) {
    begin_mut();
    priv->join = join;
}
void Pen::set_line_cap(LineCap cap) {
    begin_mut();
    priv->cap = cap;
}

bool Pen::empty() const {
    return priv == nullptr;
}
bool Pen::operator ==(const Pen &other) const {
    if (priv == other.priv) {
        return true;
    }
    return false;
}
bool Pen::operator !=(const Pen &other) const {
    return !operator ==(other);
}

auto Pen::dash_pattern() const -> DashPattern {
    if (priv) {
        return priv->dashes;
    }
    return { };
}
auto Pen::dash_offset() const -> float {
    if (priv) {
        return priv->dash_offset;
    }
    return 0.0f;
}
auto Pen::dash_style() const -> DashStyle {
    if (priv) {
        return priv->dstyle;
    }
    return DashStyle::Solid;
}
auto Pen::miter_limit() const -> float {
    if (priv) {
        return priv->miter_limit;
    }
    return 10.0f;
}
auto Pen::line_join() const -> LineJoin {
    if (priv) {
        return priv->join;
    }
    return LineJoin::Miter;
}
auto Pen::line_cap() const -> LineCap {
    if (priv) {
        return priv->cap;
    }
    return LineCap::Flat;
}

// Impl factory methods
static
auto GetPaintDeviceList() -> CompressedPtrDict<PaintDevice *(*)(void *)> & {
    static CompressedPtrDict<PaintDevice *(*)(void *)> list;
    return list;
}

auto RegisterPaintDevice(const char *type, PaintDevice *(*fn)(void *)) -> void {
    GetPaintDeviceList().push_back(type, fn);
}
auto CreatePaintDevice(const char *type, void *source) -> PaintDevice* {
    for (auto [t, create] : GetPaintDeviceList()) {
        if (::strcmp(t, type) != 0) {
            continue;
        }
        auto ret = create(source);
        if (ret) {
            return ret;
        }
    }
    return nullptr;
}

BTK_NS_END