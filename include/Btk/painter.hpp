#pragma once

#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

#include <cstdint>
#include <cstddef>
#include <vector>

BTK_NS_BEGIN

class PainterEffectImpl ;
class PainterPathImpl ;
class PainterImpl    ;
class TextureImpl    ;
class BrushImpl      ;
class PenImpl        ;

// Enum for brush type
enum class BrushType      : uint8_t {
    Solid,          //< Just a color
    LinearGradient, //< a LinearGradient
    RadialGradient, //< a RadialGradient
    Texture,        //< a gpu texture based on painter
    Bitmap,         //< Just a pixel buffer
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
enum class TextureSource : uint8_t {
    DXGISurface, //< Create texture from DXGISurface
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
        void clear() {
            _stops.clear();
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
        Brush(Color::Enum      color);
        Brush(const Brush     &brush);
        Brush(const Color     &color);
        Brush(const GLColor   &color);
        Brush(const Texture   &texture);
        Brush(const PixBuffer &pixbuf);
        Brush(const LinearGradient &gradient);
        Brush(const RadialGradient &gradient);
        ~Brush();

        void swap(Brush &);

        Brush &operator =(Brush &&);
        Brush &operator =(const Brush &);
        Brush &operator =(std::nullptr_t);

        /**
         * @brief Set the rect object (used in Bitmap / Texture brush)
         * 
         * @param r The rectangle to set width
         */
        void set_rect(const FRect &r);
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
         * @brief Set the texture object
         * 
         */
        void set_texture(const Texture &);
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
         * @brief Set the coordinate mode object
         * 
         * @param m 
         */
        void set_coordinate_mode(CoordinateMode m);
        /**
         * @brief Get the rectangle of the brush
         * 
         * @return FRect 
         */
        FRect    rect() const;
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
        /**
         * @brief Get the transform matrix of the brush
         * 
         * @return FMatrix 
         */
        FMatrix   matrix() const;
        /**
         * @brief Get the bitmap of the brush
         * 
         * @return PixBuffer 
         */
        PixBuffer bitmap() const;
        /**
         * @brief Get the abstract texture of the brush
         * 
         * @return void* 
         */
        pointer_t texture() const;
        /**
         * @brief Get the coordinate mode of the brush
         * 
         * @return CoordinateMode 
         */
        CoordinateMode coordinate_mode() const;
        /**
         * @brief Get the linear_gradient of the brush
         * 
         * @return LinearGradient 
         */
        const LinearGradient &linear_gradient() const;
        /**
         * @brief Get the radial_gradient of the brush
         * 
         * @return RadialGradient 
         */
        const RadialGradient &radial_gradient() const;

        /**
         * @brief Check if the brush is equal to another brush
         * 
         * @param brush 
         * @return true 
         * @return false 
         */
        bool operator ==(const Brush &brush) const;
        bool operator !=(const Brush &brush) const;
    public:
        /**
         * @brief Convert Point to absolute coordinates
         * 
         * @param dev The Paint Device
         * @param obj The Paint object rectangle
         * @param pt  The Point to convert
         * @return FPoint The converted point
         */
        FPoint    point_to_abs(PaintDevice *dev, const FRect &obj, const FPoint &) const;
        /**
         * @brief Convert a rectangle to absolute coordinates
         * 
         * @param dev The Paint Device
         * @param obj The Paint object rectangle
         * @param rect The rectangle to convert
         * @return FRect The converted rectangle
         */
        FRect     rect_to_abs(PaintDevice *dev, const FRect &obj, const FRect &) const;
        /**
         * @brief Bind the deviced depended resource
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @param resource The device resource (could not be nullptr)
         */
        void      bind_device_resource(void *key, PaintResource *resource) const;
        /**
         * @brief Get the deviced depended resources
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @return PaintResource * The resource, nullptr on not found
         */
        auto      query_device_resource(void *key) const -> PaintResource *;
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
         * @brief Get the dpi of the texture
         * 
         * @return FPoint 
         */
        FPoint dpi() const;
        /**
         * @brief Get the logical size of the texture
         * 
         * @return FSize 
         */
        FSize size() const;
        /**
         * @brief Get the pixel size of texture
         * 
         * @return Size 
         */
        Size pixel_size() const;
        /**
         * @brief Update data to texture
         * 
         * @param where The update area (in pixel size) , nullptr on whole texture, out of bounds is ub
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
    friend class Brush;
};


/**
 * @brief Native imaging effect for painter (maybe unavliable)
 * 
 */
// class BTKAPI PainterEffect {
//     public:
//         PainterEffect();
//         PainterEffect(EffectType type);
//         PainterEffect(PainterEffect &&);
//         ~PainterEffect();

//         PainterEffect &operator =(PainterEffect &&);
//         PainterEffect &operator =(std::nullptr_t);

//         bool attach(const Painter &p);
//         bool set_input(const Texture &tex);
//         bool set_input(const PixBuffer &buf);
//         bool set_value(EffectParam param, ...);

//         Texture output() const;
//     private:
//         PainterEffectImpl *priv;
//     friend class Painter;
// };
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
        virtual void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) = 0;

        /**
         * @brief Close the current path
         * 
         */
        virtual void close_path() = 0;

        /**
         * @brief Set the winding object (The next shape, call before move_to method)
         * 
         * @param winding 
         */
        virtual void set_winding(PathWinding winding) = 0;
};

/**
 * @brief Container of path, (implemented in indenpency way)
 * 
 */
class BTKAPI PainterPath final : public PainterPathSink {
    public:
        PainterPath();
        PainterPath(const PainterPath &);
        PainterPath(PainterPath &&);
        ~PainterPath();

        void swap(PainterPath &);

        PainterPath &operator =(PainterPath &&);
        PainterPath &operator =(const PainterPath &);
        PainterPath &operator =(std::nullptr_t);

        // Before modifying paths, call open()
        // And after modifying paths, call close()
        void open() override;
        void close() override;

        void move_to(float x, float y) override;
        void line_to(float x, float y) override;
        void bezier_to(float x1, float y1, float x2, float y2, float x3, float y3) override;
        void arc_to(float x1, float y1, float x2, float y2, float radius);
        void quad_to(float x1, float y1, float x2, float y2);
        
        // Point version
        void move_to(const FPoint &point);
        void line_to(const FPoint &point);
        void quad_to(const FPoint &point1, const FPoint &point2);
        void bezier_to(const FPoint &point1, const FPoint &point2, const FPoint &point3);

        // Helper for directly add shapes
        void add_rect(float x, float y, float w, float h);
        void add_line(float x1, float y1, float x2, float y2);

        void close_path() override;

        /**
         * @brief Clear the path
         * 
         */
        void clear();

        /**
         * @brief Check the path is empty
         * 
         * @return true 
         * @return false 
         */
        bool empty()        const;
        // Query
        /**
         * @brief Get the bounding box of the path
         * 
         * @return FRect 
         */
        FRect bounding_box() const;
        /**
         * @brief Check point is in the path
         * 
         * @param x The x of this point
         * @param y The y of this point
         * @return true 
         * @return false 
         */
        bool  contains(float x, float y) const;
        /**
         * @brief Let the content of the path stream to the sink
         * 
         * @param sink The sink of reciving this path (could not be nullptr)
         */
        void  stream(PainterPathSink *sink) const;

        // Transform
        /**
         * @brief Set the transform object
         * 
         * @param mat The transform matrix
         */
        void  set_transform(const FMatrix &mat);

        // Winding
        /**
         * @brief Set the winding object
         * 
         * @param winding The winding show that next shape show be solid or hole
         */
        void  set_winding(PathWinding winding) override;
    public:
        /**
         * @brief Bind the deviced depended resource
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @param resource The device resource (could not be nullptr)
         */
        void      bind_device_resource(void *key, PaintResource *resource) const;
        /**
         * @brief Get the deviced depended resources
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @return PaintResource * The resource, nullptr on not found
         */
        auto      query_device_resource(void *key) const -> PaintResource *;
    private:
        void begin_mut();

        PainterPathImpl *priv;
    friend class Painter;
};


/**
 * @brief Pen for stroking
 * 
 */
class BTKAPI Pen {
    public:
        using DashPattern = const std::vector<float> &;

        Pen();
        Pen(const Pen &);
        Pen(Pen &&);
        ~Pen();

        void swap(Pen &);

        Pen &operator =(Pen &&);
        Pen &operator =(const Pen &);
        Pen &operator =(std::nullptr_t);

        /**
         * @brief Set the dash pattern
         * 
         * @param pattern The pointer to a float dash array
         * @param count   The elem number of the dash array
         */
        void set_dash_pattern(const float *pattern, size_t count);
        /**
         * @brief Set the dash pattern
         * 
         * @param pattern The vector of the dash array
         */
        void set_dash_pattern(const std::vector<float> &pattern) {
            set_dash_pattern(pattern.data(), pattern.size());
        }
        /**
         * @brief Set the dash style
         * 
         * @param style The dash style of the pen, See DashStyle
         */
        void set_dash_style(DashStyle style);
        /**
         * @brief Set the dash offset
         * 
         * @param offset The dash offset
         */
        void set_dash_offset(float offset);
        /**
         * @brief Set the miter limit object
         * 
         * @param limit The limit of miter
         */
        void set_miter_limit(float limit);
        /**
         * @brief Set the line join
         * 
         * @param join 
         */
        void set_line_join(LineJoin join);
        /**
         * @brief Set the line cap
         * 
         * @param cap 
         */
        void set_line_cap(LineCap cap);

        /**
         * @brief Check the brush is empty or not
         * 
         * @return true 
         * @return false 
         */
        bool empty() const;

        DashPattern dash_pattern() const;
        DashStyle dash_style() const;
        float     dash_offset() const;
        float     miter_limit() const;
        LineJoin  line_join() const;
        LineCap   line_cap() const;

        bool operator ==(const Pen &) const;
        bool operator !=(const Pen &) const;
    public:
        /**
         * @brief Bind the deviced depended resource
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @param resource The device resource (could not be nullptr)
         */
        void      bind_device_resource(void *key, PaintResource *resource) const;
        /**
         * @brief Get the deviced depended resources
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @return PaintResource * The resource, nullptr on not found
         */
        auto      query_device_resource(void *key) const -> PaintResource *;
    private:
        void begin_mut();

        PenImpl *priv;
    friend class Painter;
};

/**
 * @brief Record the painter command 
 * 
 */
class BTKAPI PainterRecorder {
    public:
        void play(Painter &) const;
};

/**
 * @brief Draw something to target
 * 
 */
class BTKAPI Painter {
    public:
        Painter();
        /**
         * @brief Construct a new Painter object
         * 
         * @param device The PaintDevice object
         * @param owned Did the painter take ownership of the device (default false)
         */
        Painter(PaintDevice *device, bool owned = false);
        /**
         * @brief Construct a new Painter object on Texture
         * 
         * @param texture The texture
         */
        Painter(Texture   &texture);
        /**
         * @brief Construct a new Painter object on pixbuffer
         * 
         * @param buffer 
         */
        Painter(PixBuffer &buffer);

        Painter(const Painter&) = delete;
        Painter(Painter &&);
        ~Painter();

        // Configure drawing color / brush
        void set_colorf(float r, float g, float b, float a = 1.0f);
        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void set_brush(const Brush &);
        void set_font(const  Font  &);
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

        void save();
        void reset();
        void restore();

        // Draw
        void draw_rect(float x, float y, float w, float h);
        void draw_line(float x1, float y1, float x2, float y2);
        void draw_circle(float x, float y, float r);
        void draw_ellipse(float x, float y, float xr, float yr);
        void draw_rounded_rect(float x, float y, float w, float h, float r);

        void draw_rect(const FRect &);
        void draw_line(const FPoint &, const FPoint &);
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
        void set_scissor(float x, float y, float w, float h);
        void scissor(float x, float y, float w, float h);
        void reset_scissor();

        void set_scissor(const FRect &);
        void scissor(const FRect &);

        // Transform
        void transform(const FMatrix  &);
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float angle);
        void skew_x(float angle);
        void skew_y(float angle);
        void reset_transform();
        auto transform_matrix() -> FMatrix;

        // Texture
        auto create_shared_texture(TextureSource source, void *data, int w, int h, float xdpi = 96.0f, float ydpi = 96.0f) -> Texture;
        auto create_texture(PixFormat fmt, int w, int h, float xdpi = 96.0f, float ydpi = 96.0f) -> Texture;
        auto create_texture(const PixBuffer &buf)                                    -> Texture;

        // Get
        auto alpha() const -> float;
        auto context() const -> PaintContext *;

        // Interface for painter on window
        void notify_dpi_changed(float xdpi, float ydpi);
        void notify_resize(int w, int h);

        // Assign
        void operator =(Painter &&);

        // Construct painter from
        static Painter FromWindow(AbstractWindow *);
        static Painter FromPixBuffer(PixBuffer &);

        // Deprecated
        [[deprecated("Use Painter::save instead")]]
        void push_transform();
        [[deprecated("Use Painter::restore instead")]]
        void pop_transform();

        [[deprecated("Use Painter::save + Painter::scissor instead")]]
        void push_scissor(const FRect &);
        [[deprecated("Use Painter::restore instead")]]
        void pop_scissor();

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

inline void Painter::draw_rect(const FRect &r) {
    draw_rect(r.x, r.y, r.w, r.h);
}
inline void Painter::draw_line(const FPoint &from, const FPoint &to) {
    draw_line(from.x, from.y, to.x, to.y);
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

inline void Painter::scissor(const FRect &r) {
    scissor(r.x, r.y, r.w, r.h);
}
inline void Painter::set_scissor(const FRect &r) {
    set_scissor(r.x, r.y, r.w, r.h);
}

// Deprecated Painter implementation
inline void Painter::push_transform() {
    save();
}
inline void Painter::pop_transform() {
    restore();
}
inline void Painter::push_scissor(const FRect &r) {
    save();
    scissor(r);
}
inline void Painter::pop_scissor() {
    restore();
}

// Brush implementation
inline Brush::Brush(Color::Enum    c) : Brush() {
    set_color(GLColor(c));
}
inline Brush::Brush(const Color   &c) : Brush() {
    set_color(c);
}
inline Brush::Brush(const GLColor &c) : Brush() {
    set_color(c);
}
inline Brush::Brush(const Texture &t) : Brush() {
    set_texture(t);
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

// Lerp
constexpr inline ColorStop lerp(const ColorStop &a, const ColorStop &b, float t) noexcept {
    return ColorStop {
        lerp(a.offset, b.offset, t),
        lerp(a.color, b.color, t)
    };
}

BTK_NS_END