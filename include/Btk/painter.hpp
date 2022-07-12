#pragma once

#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

#include <cstdint>
#include <cstddef>
#include <vector>

BTK_NS_BEGIN

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

class ColorStop {
    public:
        float  offset; // normalized offset from [0.0 => 1.0]
        GLColor color;
};

class Gradient {
    public:
        Gradient() = default;

        void add_stop(float offset, const GLColor &color) {
            _stops.push_back(ColorStop{offset, color});
        }

        auto stops() const -> const std::vector<ColorStop> & {
            return _stops;
        }
    protected:
        std::vector<ColorStop> _stops = {};
};
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
    protected:
        FPoint  _start_point = {0.0f, 0.0f};
        FPoint  _end_point   = {1.0f, 1.0f};
};
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
    private:
        FPoint _origin_offset = {0.0f, 0.0f};
        FPoint _center_point  = {0.5f, 0.5f};
        float  _radius_x      =  0.5f;
        float  _radius_y      =  0.5f;
};

class Brush {
    public:
        Brush();
        Brush(const Brush &);
        Brush(Brush &&);
        ~Brush();

        void swap(Brush &);

        Brush &operator =(Brush &&);
        Brush &operator =(const Brush &);
        Brush &operator =(std::nullptr_t);

        void set_color(GLColor c);
        void set_image(const PixBuffer &);
        void set_gradient(const LinearGradient &g);
        void set_gradient(const RadialGradient &g);

        BrushType type() const;
        
    private:
        void begin_mut();

        BrushImpl *priv;
    friend class Painter;
};

class Texture {
    public:
        Texture();
        Texture(Texture &&);
        ~Texture();

        void clear();
        bool empty() const;
        Size size() const;
        void update(const Rect *where, cpointer_t data, uint32_t pitch);

        Texture &operator =(Texture      &&);
        Texture &operator =(std::nullptr_t );
    private:
        TextureImpl *priv;
    friend class Painter;
};

class TextLayout {
    public:
        TextLayout();
        TextLayout(const TextLayout &);
        TextLayout(TextLayout &&);
        ~TextLayout();

        void swap(TextLayout &);

        TextLayout &operator =(TextLayout &&);
        TextLayout &operator =(const TextLayout &);
        TextLayout &operator =(std::nullptr_t);

        void set_text(u8string_view text);
        void set_font(const Font &);
        
        Size size() const;

    private:
        void begin_mut();

        TextLayoutImpl *priv;
    friend class Painter;
};

class Font {
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

        static Font FromFile(u8string_view fname, float size);
    private:
        void begin_mut();

        FontImpl *priv;
    friend class TextLayout;
    friend class Painter;
};

class Pen {
    private:
        PenImpl *priv;
    friend class Painter;
};

class PainterImpl ;
class Painter {
    public:
        Painter();
        Painter(const Painter&) = delete;
        Painter(Painter &&);
        ~Painter();

        // Configure drawing color / brush
        void set_colorf(float r, float g, float b, float a = 1.0f);
        void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void set_brush(const Brush &);
        void set_font(const  Font  &);
        void set_stroke_width(float width);
        void set_text_align(align_t align);
        void set_antialias(bool enable);

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
        
        void draw_text(const TextLayout &lay, float x, float y);        
        void draw_text(u8string_view txt, float x, float y);

        // Fill
        void fill_rect(float x, float y, float w, float h);
        void fill_circle(float x, float y, float r);
        void fill_ellipse(float x, float y, float xr, float yr);
        void fill_rounded_rect(float x, float y, float w, float h, float r);

        void fill_rect(const FRect &);
        void fill_rounded_rect(const FRect &, float r);

        // Scissor
        void set_scissor(float x, float y, float w, float h);
        void intersect_scissor(float x, float y, float w, float h);
        void reset_scissor();

        void set_scissor(const FRect &);
        void intersect_scissor(const FRect &);

        // Vector Graphics
        void begin_path();
        void close_path();
        void end_path();

        // Transform
        void translate(float x, float y);
        void scale(float x, float y);
        void rotate(float angle);
        void skew_x(float angle);
        void skew_y(float angle);
        void reset_transform();

        // Texture
        auto create_texture(PixFormat fmt, int w, int h) -> Texture;

        // Notify 
        void notify_resize(int w, int h); //< Target size changed

        // Assign
        void operator =(Painter &&);

        // Construct painter from 
        static Painter FromHwnd(void * hwnd);
        static Painter FromHdc(void * hdc);
        static Painter FromXlib(void * dpy, void * win);
        static Painter FromXcb(void * dpy, void * win);
        static Painter FromPixBuffer(PixBuffer &);

        // Initialize painter internal state
        static void Init();
        static void Shutdown();
    private:
        PainterImpl *priv; // Private implementation
};

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
inline void Painter::draw_rounded_rect(const FRect &r, float rad) {
    draw_rounded_rect(r.x, r.y, r.w, r.h, rad);
}

inline void Painter::fill_rect(const FRect &r) {
    fill_rect(r.x, r.y, r.w, r.h);
}
inline void Painter::fill_rounded_rect(const FRect &r, float rad) {
    fill_rounded_rect(r.x, r.y, r.w, r.h, rad);
}

inline void Painter::set_scissor(const FRect &r) {
    set_scissor(r.x, r.y, r.w, r.h);
}
inline void Painter::intersect_scissor(const FRect &r) {
    intersect_scissor(r.x, r.y, r.w, r.h);
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