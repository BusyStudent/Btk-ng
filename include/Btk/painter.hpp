#pragma once

#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

#include <cstdint>
#include <cstddef>
#include <vector>

BTK_NS_BEGIN

class PainterEffectImpl ;
class PainterPathImpl ;
class TextLayoutImpl ;
class PainterImpl    ;
class TextureImpl    ;
class BrushImpl      ;
class FontImpl       ;
class PenImpl        ;

// Enum for brush type
enum class BrushType      : uint8_t {
    Solid,
    LinearGradient,
    RadialGradient,
    Texture,
};
enum class CoordinateMode : uint8_t {
    Absolute, // Absolute coordinates
    Relative, // Depends on the current drawing object
    Device,   // Depends on the device
};
enum class PainterFeature : uint8_t {
    MultiStopGradient, //< Gradient with multiple stops (greater than 2)
    TextGradient,      //< Does text support gradient?
    Antialias,         //< Does the painter support antialiasing?
    Gradient,          //< Does the painter support gradient?
    Effect,            //< Does the painter support effect?
    Dash,              //< Does the painter support dash?
    Path,              //< Does the painter support rendering paths?
};
enum class EffectParam : uint8_t {
    BlurStandardDeviation, //< Blur : type float
};
enum class EffectType  : uint8_t {
    Blur,
};
enum class PathWinding : uint8_t {
    CW,  //< Odd-even rule
    CCW, //< Non-zero rule

    Solid = CW,
    Hole  = CCW,
};
enum class DashStyle : uint8_t {
    Solid,
    Dash,
    Dot,
    DashDot,
    DashDotDot,
    Custom, //< User defined
};
enum class FontStyle : uint8_t {
    Normal = 0,
    Bold   = 1 << 0,
    Italic = 1 << 1,
};
enum class LineJoin : uint8_t {
    Miter,
    Bevel,
    Round,
};
enum class LineCap : uint8_t {
    Flat,
    Round,
    Square,
};

BTK_FLAGS_OPERATOR(FontStyle, uint8_t);

/**
 * @brief Color stop structore
 * 
 */
class ColorStop {
    public:
        float  offset; //< normalized offset from [0.0 => 1.0]
        GLColor color;

        bool compare(const ColorStop &other) const {
            return Btk_memcmp(this, &other, sizeof(ColorStop)) == 0;
        }
        bool operator ==(const ColorStop &other) const {
            return compare(other);
        }
        bool operator !=(const ColorStop &other) const {
            return !compare(other);
        }
};

/**
 * @brief The Gradient 
 * 
 */
class Gradient {
    public:
        Gradient() = default;

        void add_stop(float offset, const GLColor &color) {
            _stops.push_back(ColorStop{offset, color});
        }

        auto stops() const -> const std::vector<ColorStop> & {
            return _stops;
        }

        // Compare 
        bool compare(const Gradient &other) const {
            if (_stops.size() != other._stops.size()) {
                return false;
            }
            // Because is vector
            auto size = sizeof(ColorStop) * _stops.size();
            return Btk_memcmp(_stops.data(), other._stops.data(), size) == 0;
        }
        bool operator ==(const Gradient &other) const {
            return compare(other);
        }
        bool operator !=(const Gradient &other) const {
            return !compare(other);
        }
    protected:
        std::vector<ColorStop> _stops = {};
};
/**
 * @brief LinearGradient
 * 
 */
class LinearGradient : public Gradient {
    public:
        LinearGradient() = default;

        void set_start_point(float x, float y) {
            _start_point = {x, y};
        }
        void set_end_point(float x, float y) {
            _end_point = {x, y};
        }
        void set_start_point(const FPoint &point) {
            _start_point = point;
        }
        void set_end_point(const FPoint &point) {
            _end_point = point;
        }

        // Query
        FPoint  start_point() const { return _start_point; }
        FPoint  end_point()   const { return _end_point; }

        // Compare
        bool    compare(const LinearGradient &other) const {
            if (!Gradient::compare(other)) {
                return false;
            }
            return _start_point == other._start_point && _end_point == other._end_point;
        }
        bool    operator ==(const LinearGradient &other) const {
            return compare(other);
        }
        bool    operator !=(const LinearGradient &other) const {
            return !compare(other);
        }
    protected:
        FPoint  _start_point = {0.0f, 0.0f};
        FPoint  _end_point   = {1.0f, 1.0f};
};
/**
 * @brief RadialGradient
 * 
 */
class RadialGradient : public Gradient {
    public:
        // Has Start and End points
        RadialGradient() = default;
        
        void set_center_point(float x, float y) {
            _center_point = {x, y};
        }
        void set_center_point(const FPoint &point) {
            _center_point = point;
        }
        void set_origin_offset(float x, float y) {
            _origin_offset = {x, y};
        }
        void set_origin_offset(const FPoint &point) {
            _origin_offset = point;
        }
        void set_radius(float radius) {
            _radius_x = radius;
            _radius_y = radius;
        }
        void set_radius(float radius_x, float radius_y) {
            _radius_x = radius_x;
            _radius_y = radius_y;
        }
        void set_radius_x(float radius_x) {
            _radius_x = radius_x;
        }
        void set_radius_y(float radius_y) {
            _radius_y = radius_y;
        }

        // Query
        FPoint  center_point() const { return _center_point; }
        FPoint  origin_offset() const { return _origin_offset; }
        float   radius_x()     const { return _radius_x; }
        float   radius_y()     const { return _radius_y; }

        // Compare
        bool    compare(const RadialGradient &other) const {
            if (!Gradient::compare(other)) {
                return false;
            }
            return _center_point == other._center_point && _origin_offset == other._origin_offset &&
                   _radius_x == other._radius_x && _radius_y == other._radius_y;
        }
        bool    operator ==(const RadialGradient &other) const {
            return compare(other);
        }
        bool    operator !=(const RadialGradient &other) const {
            return !compare(other);
        }
    private:
        FPoint _origin_offset = {0.0f, 0.0f};
        FPoint _center_point  = {0.5f, 0.5f};
        float  _radius_x      =  0.5f;
        float  _radius_y      =  0.5f;
};

/**
 * @brief Brush for filling
 * 
 */
class BTKAPI Brush {
    public:
        Brush();
        Brush(Brush &&);
        Brush(const Brush     &brush);
        Brush(const GLColor   &color);
        Brush(const PixBuffer &pixbuf);
        Brush(const LinearGradient &gradient);
        Brush(const RadialGradient &gradient);
        ~Brush();

        void swap(Brush &);

        Brush &operator =(Brush &&);
        Brush &operator =(const Brush &);
        Brush &operator =(std::nullptr_t);

        /**
         * @brief Set the color object
         * 
         * @param c The color
         */
        void set_color(const GLColor &c);
        /**
         * @brief Set the image object
         * 
         * @param pixbuf The const reference of pixbuffer
         */
        void set_image(const PixBuffer &);
        /**
         * @brief Set the gradient object
         * 
         * @param g LinearGradient
         */
        void set_gradient(const LinearGradient &g);
        /**
         * @brief Set the gradient object
         * 
         * @param g RadialGradient
         */
        void set_gradient(const RadialGradient &g);

        /**
         * @brief Get current type of the brush
         * 
         * @return BrushType 
         */
        BrushType type() const;
        /**
         * @brief Get color of the brush
         * 
         * @return GLColor (failed will return Color::Black)
         */
        GLColor   color() const;
    private:
        void begin_mut();

        BrushImpl *priv;
    friend class Painter;
};

/**
 * @brief Texture for direct copy to GPU( render target )
 * 
 */
class BTKAPI Texture {
    public:
        Texture();
        Texture(Texture &&);
        ~Texture();

        /**
         * @brief Clear the texture, make it into empty
         * 
         */
        void clear();
        /**
         * @brief Check the texture is empty
         * 
         * @return true 
         * @return false 
         */
        bool empty() const;
        /**
         * @brief Get the pixel size of texture
         * 
         * @return Size 
         */
        Size size() const;
        /**
         * @brief Update data to texture
         * 
         * @param where The update area, nullptr on whole texture, out of bounds is ub
         * @param data The pointer of pixel data
         * @param pitch The pitch of pixel data
         */
        void update(const Rect *where, cpointer_t data, uint32_t pitch);
        /**
         * @brief Set the interpolation mode object
         * 
         * @param mode InterpolationMode
         */
        void set_interpolation_mode(InterpolationMode mode);

        Texture &operator =(Texture      &&);
        Texture &operator =(std::nullptr_t );
    private:
        TextureImpl *priv;
    friend class PainterEffect;
    friend class Painter;
};

/**
 * @brief Hit results for TextLayout
 * 
 */
class TextHitResult {
    public:
        // Properties unused in hit_test_range()
        bool   inside;
        bool   trailing;

        size_t text; //< Text position
        size_t length; //< Text length
        FRect  box; //< Bounding box
};
/**
 * @brief Line's metrics for TextLayout
 * 
 */
class TextLineMetrics {
    public:
        float height;   //< Line's height
        float baseline; //< Line's baseline
};

using TextHitResults = std::vector<TextHitResult>;
using TextHitResultList = std::vector<TextHitResult>;
using TextLineMetricsList = std::vector<TextLineMetrics>;

/**
 * @brief For analizing and drawing text
 * 
 */
class BTKAPI TextLayout {
    public:
        TextLayout();
        TextLayout(const TextLayout &);
        TextLayout(TextLayout &&);
        ~TextLayout();

        void swap(TextLayout &);

        TextLayout &operator =(TextLayout &&);
        TextLayout &operator =(const TextLayout &);
        TextLayout &operator =(std::nullptr_t);

        /**
         * @brief Set the text object
         * 
         * @param text 
         */
        void set_text(u8string_view text);
        /**
         * @brief Set the font object
         * 
         */
        void set_font(const Font &);

        /**
         * @brief Get the size of the text block
         * 
         * @return Size 
         */
        FSize  size() const;
        /**
         * @brief Get how many lines
         * 
         * @return size_t 
         */
        size_t line() const;

        /**
         * @brief Hit test by mouse position
         * 
         * @param x logical x pos (begin at 0)
         * @param y logical y pos (begin at 0)
         * @param res The pointer to hit result
         * @return true 
         * @return false 
         */
        bool hit_test(float x, float y, TextHitResult *res = nullptr) const;
        /**
         * @brief Hit test for single char
         * 
         * @param pos The char position
         * @param trailing_hit 
         * @param x 
         * @param y 
         * @param res 
         * @return true 
         * @return false 
         */
        bool hit_test_pos(size_t pos, bool trailing_hit, float *x, float *y, TextHitResult *res = nullptr) const;
        /**
         * @brief Hit test for text block
         * 
         * @param text The text position
         * @param len The length of text
         * @param org_x The x position of the text layout
         * @param org_y The y position of the text layout
         * @param res The pointer to hit results
         * @return true 
         * @return false 
         */
        bool hit_test_range(size_t text, size_t len, float org_x, float org_y, TextHitResults *res = nullptr) const;
        /**
         * @brief Get lines's metrics
         * 
         * @param metrics Pointer to vector<TextLineMetrics>
         * @return true 
         * @return false 
         */
        bool line_metrics(TextLineMetricsList *metrics = nullptr) const;
    private:
        void begin_mut();

        TextLayoutImpl *priv;
    friend class Painter;
};

/**
 * @brief Native imaging effect for painter (maybe unavliable)
 * 
 */
class BTKAPI PainterEffect {
    public:
        PainterEffect();
        PainterEffect(EffectType type);
        PainterEffect(PainterEffect &&);
        ~PainterEffect();

        PainterEffect &operator =(PainterEffect &&);
        PainterEffect &operator =(std::nullptr_t);

        bool attach(const Painter &p);
        bool set_input(const Texture &tex);
        bool set_input(const PixBuffer &buf);
        bool set_value(EffectParam param, ...);

        Texture output() const;
    private:
        PainterEffectImpl *priv;
    friend class Painter;
};

/**
 * @brief Receiver of path (method object)
 * 
 */
class PainterPathSink {
    public:
        virtual void open() = 0;
        virtual void close() = 0;

        virtual void move_to(float x, float y) = 0;
        virtual void line_to(float x, float y) = 0;
        virtual void quad_to(float x1, float y1, float x2, float y2) = 0;
        virtual void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) = 0;
        virtual void arc_to(float x1, float y1, float x2, float y2, float radius) = 0;

        virtual void close_path() = 0;
};

/**
 * @brief Container of path, (implemented in native way)
 * 
 */
class BTKAPI PainterPath : public PainterPathSink {
    public:
        PainterPath();
        PainterPath(PainterPath &&);
        ~PainterPath();

        // Before modifying paths, call open()
        // And after modifying paths, call close()
        void open() override;
        void close() override;

        void move_to(float x, float y) override;
        void line_to(float x, float y) override;
        void quad_to(float x1, float y1, float x2, float y2) override;
        void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) override;
        void arc_to(float x1, float y1, float x2, float y2, float radius) override;

        // Point version
        void move_to(const FPoint &point);
        void line_to(const FPoint &point);
        void quad_to(const FPoint &point1, const FPoint &point2);
        void bezier_to(const FPoint &point1, const FPoint &point2, const FPoint &point3);

        // Helper for directly add shapes
        void add_rect(float x, float y, float w, float h);
        void add_line(float x1, float y1, float x2, float y2);

        void close_path() override;
        
        void clear();

        // Query
        FRect bounding_box() const;
        bool  contains(float x, float y) const;
        void  stream(PainterPathSink *sink) const;

        // Transform
        void  set_transform(const FMatrix &mat);

        PainterPath &operator =(PainterPath &&);
    private:
        PainterPathImpl *priv;
    friend class Painter;
};

/**
 * @brief Logical font description
 * 
 */
class BTKAPI Font {
    public:
        Font();
        Font(u8string_view s, float size);
        Font(const Font &);
        Font(Font &&);
        ~Font();

        void swap(Font &);

        Font &operator =(Font &&);
        Font &operator =(const Font &);
        Font &operator =(std::nullptr_t);

        float size() const;
        bool  empty() const;
        void  set_size(float size);
        void  set_bold(bool bold);
        void  set_italic(bool italic);
        void  set_family(u8string_view family);

        static auto FromFile(u8string_view fname, float size) -> Font;
        static auto ListFamily()                              -> StringList;
    private:
        void begin_mut();

        FontImpl *priv;
    friend class TextLayout;
    friend class Painter;
};

/**
 * @brief Pen for stroking
 * 
 */
class BTKAPI Pen {
    public:
        Pen();
        Pen(const Pen &);
        Pen(Pen &&);
        ~Pen();

        void swap(Pen &);

        Pen &operator =(Pen &&);
        Pen &operator =(const Pen &);
        Pen &operator =(std::nullptr_t);

        void set_dash_pattern(const float *pattern, size_t count);
        void set_dash_pattern(const std::vector<float> &pattern) {
            set_dash_pattern(pattern.data(), pattern.size());
        }
        void set_dash_pattern(float pattern) {
            set_dash_pattern(&pattern, 1);
        }
        void set_dash_style(DashStyle style);
        void set_dash_offset(float offset);
        void set_miter_limit(float limit);
        void set_line_join(LineJoin join);
        void set_line_cap(LineCap cap);
    private:
        void begin_mut();

        PenImpl *priv;
    friend class Painter;
};

/**
 * @brief Draw something to target
 * 
 */
class BTKAPI Painter {
    public:
        Painter();
        Painter(Texture &);
        Painter(PixBuffer &);
        Painter(const Painter&) = delete;
        Painter(Painter &&);
        ~Painter();

        // Configure drawing color / brush
        void set_colorf(float r, float g, float b, float a = 1.0f);
        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void set_brush(const Brush &);
        void set_font(const  Font  &);
        //< Pass nullptr to disable pen
        void set_pen(const Pen *); 
        void set_pen(const Pen &);
        void set_stroke_width(float width);
        void set_text_align(align_t align);
        void set_antialias(bool enable);
        void set_alpha(float alpha);

        void set_color(GLColor c);
        void set_color(Color   c);
        void set_color(Color::Enum e);

        void begin();
        void end();
        void clear();

        // Draw
        void draw_rect(float x, float y, float w, float h);
        void draw_line(float x1, float y1, float x2, float y2);
        void draw_circle(float x, float y, float r);
        void draw_ellipse(float x, float y, float xr, float yr);
        void draw_rounded_rect(float x, float y, float w, float h, float r);

        void draw_rect(const FRect &);
        void draw_lines(const FPoint *fp, size_t n);
        void draw_rounded_rect(const FRect &, float r);
        
        void draw_image(const Texture &tex, const FRect *dst, const FRect *src);
        void draw_text(const TextLayout &lay, float x, float y);        
        void draw_text(u8string_view txt, float x, float y);
        void draw_path(const PainterPath &path);
        
        // Fill
        void fill_rect(float x, float y, float w, float h);
        void fill_circle(float x, float y, float r);
        void fill_ellipse(float x, float y, float xr, float yr);
        void fill_rounded_rect(float x, float y, float w, float h, float r);

        void fill_rect(const FRect &);
        void fill_rounded_rect(const FRect &, float r);
        void fill_path(const PainterPath &paths);
        void fill_mask(const Texture &mask, const FRect *dst, const FRect *src);

        // Scissor
        void push_scissor(float x, float y, float w, float h);
        void push_scissor(const FRect &);
        void pop_scissor();


        // Vector Graphics
        void begin_path();
        void close_path();
        void end_path();

        // Transform
        void transform(const FMatrix  &);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float angle);
        void skew_x(float angle);
        void skew_y(float angle);
        void reset_transform();
        void push_transform();
        void pop_transform();
        auto transform_matrix() -> FMatrix;

        // Texture
        auto create_texture(PixFormat fmt, int w, int h) -> Texture;
        auto create_texture(const PixBuffer &buf)        -> Texture;

        // Get color / brush / alpha
        auto pen()   const -> Pen;
        auto brush() const -> Brush;
        auto color() const -> GLColor;
        auto alpha() const -> float;

        // Target
        void push_group(int w, int h);
        void pop_group_to();

        // Notify 
        void notify_resize(int w, int h); //< Target size changed

        // Assign
        void operator =(Painter &&);

        // Construct painter from 
        static Painter FromWindow(AbstractWindow *window);
        static Painter FromDxgiSurface(void *surface);
        static Painter FromHwnd(void * hwnd);
        static Painter FromHdc(void * hdc);
        static Painter FromXlib(void * dpy, void * win);
        static Painter FromXcb(void * dpy, void * win);
        static Painter FromPixBuffer(PixBuffer &);
        static Painter FromTexture(Texture &);

        // Getters
        static bool HasFeature(PainterFeature f);

        // Initialize painter internal state
        static void Init();
        static void Shutdown();
    private:
        // Internal Constructor
        Painter(PainterImpl *p) : priv(p) {}

        PainterImpl *priv; // Private implementation
    friend class PainterEffect;
};

/**
 * @brief Helper class for RAII init painter
 * 
 */
class PainterInitializer {
    public:
        PainterInitializer() {
            Painter::Init();
        }
        ~PainterInitializer() {
            Painter::Shutdown();
        }
};

// Painter implementation
inline void Painter::set_color(GLColor c) {
    set_colorf(c.r, c.g, c.b, c.a);
}
inline void Painter::set_color(Color c) {
    set_color(c.r, c.g, c.b, c.a);
}
inline void Painter::set_color(Color::Enum e) {
    set_color(GLColor(e));
}
inline void Painter::set_pen(const Pen &p) {
    set_pen(&p);
}

inline void Painter::draw_rect(const FRect &r) {
    draw_rect(r.x, r.y, r.w, r.h);
}
inline void Painter::draw_rounded_rect(const FRect &r, float rad) {
    draw_rounded_rect(r.x, r.y, r.w, r.h, rad);
}

inline void Painter::fill_rect(const FRect &r) {
    fill_rect(r.x, r.y, r.w, r.h);
}
inline void Painter::fill_rounded_rect(const FRect &r, float rad) {
    fill_rounded_rect(r.x, r.y, r.w, r.h, rad);
}

inline void Painter::push_scissor(const FRect &r) {
    push_scissor(r.x, r.y, r.w, r.h);
}

// Brush implementation
inline Brush::Brush(const GLColor &c) : Brush() {
    set_color(c);
}
inline Brush::Brush(const PixBuffer &buf) : Brush() {
    set_image(buf);
}
inline Brush::Brush(const LinearGradient &grad) : Brush() {
    set_gradient(grad);
}
inline Brush::Brush(const RadialGradient &grad) : Brush() {
    set_gradient(grad);
}

// PainterPath implementation
inline void PainterPath::move_to(const FPoint &p) {
    move_to(p.x, p.y);
}
inline void PainterPath::line_to(const FPoint &p) {
    line_to(p.x, p.y);
}
inline void PainterPath::quad_to(const FPoint &p1, const FPoint &p2) {
    quad_to(p1.x, p1.y, p2.x, p2.y);
}
inline void PainterPath::bezier_to(const FPoint &p1, const FPoint &p2, const FPoint &p3) {
    bezier_to(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

inline void PainterPath::add_rect(float x, float y, float w, float h) {
    move_to(x, y);
    line_to(x + w, y);
    line_to(x + w, y + h);
    line_to(x, y + h);
    close_path();
}
inline void PainterPath::add_line(float x1, float y1, float x2, float y2) {
    move_to(x1, y1);
    line_to(x2, y2);
}


// Platform specific declarations
#if defined(_WIN32)
namespace Win32 {
    BTKAPI void *WicFactory();
    BTKAPI void *D2dFactory();
    BTKAPI void *DWriteFactory();
}
#endif


BTK_NS_END