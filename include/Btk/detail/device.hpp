/**
 * @file device.hpp
 * @author BusyStudent (fyw90mc@gmail.com)
 * @brief Abstractions of the paint interface
 * @version 0.1
 * @date 2023-01-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// TODO : Need to implement

#pragma once

#include <Btk/detail/macro.hpp>
#include <Btk/rect.hpp>

// Using RTTI to Get the type of it
#if !defined(BTK_NO_RTTI)
#include <type_traits>
#include <typeinfo>
#endif

BTK_NS_BEGIN

class PaintDevice;
class PaintContext;
class AbstractTexture;

enum class PaintContextFeature : uint8_t {
    Pen,         //< Supports pen style
    FillMask,    //< Supports fill mask
    FillPath,    //< Supports fill path
    DrawPath,    //< Supports draw path
    GradientBrush, //< Supports gradient
};
enum class PaintContextState  : uint8_t {
    Antialias,   //< The antialiasing mode (bool  *) true    on enabled
    Transform,   //< The Transform matrix (FMatrix *) nullptr on Identity
    Scissor,     //< The scissor to clip  (FRect *)   nullptr on no scissor
    Alpha,       //< The alpha value      (float)
};
enum class PaintContextHandle : uint8_t {
    ID2D1RenderTarget,
};
enum class PaintDeviceValue   : uint8_t {
    LogicalSize, //< Logical pixel size of the drawable (FSize)
    PixelSize,   //< Pixel size of the drawable         (Size)
    Dpi,         //< Dpi of the drawable                (FPoint)
    PixelFormat, //< Pixel format of the drawable       (PixFormat)
};

class GLFormat {
    public:
        uint8_t red_size     = 8;
        uint8_t green_size   = 8;
        uint8_t blue_size    = 8;
        uint8_t alpha_size   = 8;
        uint8_t depth_size   = 24;
        uint8_t stencil_size = 8;
        uint8_t buffer_count = 1; //< Double buffer or etc.
        //< MSAA Fields
        uint8_t samples = 1;
        uint8_t sample_buffers = 0;
};

/**
 * @brief Graphics Context like OpenGL
 * 
 */
class GraphicsContext : public Any {
    public:
        virtual void begin() = 0;
        virtual void end() = 0;
        virtual void swap_buffers() = 0;
};
/**
 * @brief Destination of paint, query information & create the paint context
 * 
 */
class PaintDevice {
    public:
        virtual ~PaintDevice() {}

        virtual auto  paint_context() -> PaintContext* = 0;
        virtual bool  query_value(PaintDeviceValue value, void *) = 0;

        // Simplify query value for user
        inline  FSize size() {
            FSize result {0.0f, 0.0f};
            query_value(PaintDeviceValue::LogicalSize, &result);
            return result;
        }
        inline  FPoint dpi() {
            FPoint dpi  {96.0f, 96.0f};
            query_value(PaintDeviceValue::Dpi, &dpi);
            return dpi;
        }
        inline Size    pixel_size() {
            Size result {0, 0};
            query_value(PaintDeviceValue::PixelSize, &result);
            return result;
        }
};
/**
 * @brief Interface for managing resource lifetime
 * 
 */
class PaintResource {
    public:
        virtual void ref() = 0;
        virtual void unref() = 0;

        /**
         * @brief Signal reported that this resources is no longer available
         * 
         * @return Signal<void()>& 
         */
        virtual auto signal_destroyed() -> Signal<void()>& = 0;
};
/**
 * @brief Paint Context
 * 
 */
class PaintContext : public PaintResource, public GraphicsContext {
    public:
        // Clear
        virtual void clear(Brush &) = 0;

        // Draw
        virtual bool draw_path(PainterPath &path, Brush &brush, float stroke_width, Pen &pen) = 0;
        virtual bool draw_line(FPoint from, FPoint to, Brush &brush, float stroke_width, Pen &pen) = 0;
        virtual bool draw_rect(FRect rect, Brush &brush, float stroke_width, Pen &pen) = 0;
        virtual bool draw_rounded_rect(FRect rect, float r, Brush &brush, float stroke_width, Pen &pen) = 0;
        virtual bool draw_ellipse(FPoint center, float xr, float yr, Brush &brush, float stroke_width, Pen &pen) = 0;
        virtual bool draw_image(AbstractTexture *image, const FRect *dst, const FRect *src) = 0;

        // Text
        virtual bool draw_text(Alignment, Font &font, u8string_view text, float x, float y, Brush &) = 0;
        virtual bool draw_text(Alignment, TextLayout &layout            , float x, float y, Brush &) = 0;

        // Fill
        virtual bool fill_path(PainterPath &path, Brush &brush) = 0;
        virtual bool fill_rect(FRect rect, Brush &brush) = 0;
        virtual bool fill_rounded_rect(FRect rect, float r, Brush &brush) = 0;
        virtual bool fill_ellipse(FPoint center, float xr, float yr, Brush &brush) = 0;
        virtual bool fill_mask(AbstractTexture *mask, const FRect *dst, const FRect *src) = 0;

        // Texture
        /**
         * @brief Create a texture object
         * 
         * @param fmt The Pixel format
         * @param w The pixel width of this texture
         * @param h The pixel height of this texture
         * @param xdpi The xdpi of this texture (0 on device xdpi)
         * @param ydpi The ydpi of this texture (0 on device ydpi)
         * @return AbstractTexture* 
         */
        virtual auto create_texture(PixFormat fmt, int w, int h, float xdpi = 96, float ydpi = 96) -> AbstractTexture * = 0;
        
        /**
         * @brief Check the context supports given feature
         * 
         * @param feature 
         * @return true 
         * @return false 
         */
        virtual bool has_feature(PaintContextFeature feature) = 0;

        /**
         * @brief Get the native_handle of it
         * 
         * @param what 
         * @param out 
         * @return true 
         * @return false 
         */
        virtual bool native_handle(PaintContextHandle what, void *out) = 0;

        /**
         * @brief Set the state object
         * 
         * @param state The PaintContextState enum value
         * @param v The value to set
         * @return true 
         * @return false 
         */
        virtual bool set_state(PaintContextState state, const void *v) = 0;

        /**
         * @brief Set the transform object
         * 
         * @param mat The transform matrix, nullptr on Identity matrix
         * @return true 
         * @return false 
         */
        inline  bool set_transform(const FMatrix *mat) {
            return set_state(PaintContextState::Transform, mat);
        }

        /**
         * @brief Set the scissor object
         * 
         * @param rect The scissor rectangle, nullptr on no scissor
         * @return true 
         * @return false 
         */
        inline  bool set_scissor(const FRect *rect) {
            return set_state(PaintContextState::Scissor, rect);
        }

        inline  bool set_alpha(float alpha) {
            return set_state(PaintContextState::Alpha, &alpha);
        }

        inline bool set_antialias(bool enabled) {
            return set_state(PaintContextState::Antialias, &enabled);
        }
};

/**
 * @brief Native Device depended texture
 * 
 */
class AbstractTexture : public PaintResource, public PaintDevice {
    public:
        /**
         * @brief Copy the pixels to the texture
         * 
         * @param rect The rectangle of the texture, in pixels size
         * @param data 
         * @param pitch 
         */
        virtual void update(const Rect *rect, cpointer_t data, int pitch) = 0;
};

/**
 * @brief OpenGL graphics context.
 * 
 */
class GLContext : public GraphicsContext {
    public:
        virtual bool      initialize(const GLFormat &) = 0;
        virtual bool      make_current() = 0;
        virtual bool      set_swap_interval(int v) = 0;
        virtual Size      get_drawable_size() = 0;
        virtual pointer_t get_proc_address(const char_t *name) = 0;
};

template <typename T = PaintResource>
inline T    *PaintResourceFromPtr(void *ptr) noexcept {
    return static_cast<T *>(static_cast<PaintResource *>(ptr));
}
inline void *PaintResourceToPtr(PaintResource *ptr) noexcept {
    return static_cast<void *>(ptr);
}

// Factroy Interfaces
/**
 * @brief Create a Paint Device object
 * 
 * @param types The string of the types
 * @param source The source to create the factory
 * @return PaintDevice* nullptr on failure or not founded
 */
BTKAPI auto CreatePaintDevice(const char *types, void *source) -> PaintDevice *;
/**
 * @brief Register a create factory function
 * 
 * @param type Thehe string of the types
 * @param fn The function to create (could not be nullptr)
 */
BTKAPI auto RegisterPaintDevice(const char *type, PaintDevice *(*fn)(void *source)) -> void;

/**
 * @brief Create a Paint Device object
 * 
 * @tparam T The type of source
 * @param source The pointer to source
 * @return PaintDevice* 
 */
template <typename T>
inline auto CreatePaintDevice(T *source) -> PaintDevice *{
    using Type = std::decay_t<T>;
    return CreatePaintDevice(typeid(Type).name(), source);
}
template <typename T>
inline auto RegisterPaintDevice(PaintDevice *(*fn)(std::conditional_t<std::is_pointer_v<T>, T, T*> source)) -> void {
    using Type = std::decay_t<T>;
    return RegisterPaintDevice(typeid(Type).name(), reinterpret_cast<PaintDevice *(*)(void *)>(fn));
}

BTK_NS_END