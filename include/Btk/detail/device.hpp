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

BTK_NS_BEGIN

class PaintDevice;
class PaintContext;

enum class PaintContextFeature : uint8_t {
    DirectBrush, //< Without create native brush,
    DirectPath,  //< Without create native path
    DirectPen,   //< Without create native pen
    FillMask,    //< Supports fill mask
    FillPath,    //< Supports fill path
    DrawPath,    //< Supports draw path
};
enum class PaintNativeContext : uint8_t {
    ID2D1RenderTarget,
};
enum class PaintDeviceValue   : uint8_t {
    LogicalSize, //< Logical pixel size of the drawable (FSize)
    PixelSize,   //< Pixel size of the drawable         (Size)
    Dpi,         //< Dpi of the drawable                (FPoint)
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
        virtual void      begin() = 0;
        virtual void      end() = 0;
        virtual void      swap_buffers() = 0;
};
/**
 * @brief Destination of paint
 * 
 */
class PaintDevice {
    public:
        virtual PaintContext *paint_context() = 0;

        virtual bool          query_value() = 0;
};
/**
 * @brief Paint Context
 * 
 */
class PaintContext : public GraphicsContext {
    public:

        // Draw
        virtual bool draw_path(void *path, void *brush, float stroke_width, void *pen) = 0;
        virtual bool draw_line(FPoint fp, void *brush, float stroke_width, void *pen) = 0;
        virtual bool draw_rect(FRect rect, void *brush, float stroke_width, void *pen) = 0;
        virtual bool draw_ellipse(FPoint center, float xr, float yr, void *brush, float stroke_width, void *pen) = 0;
        virtual bool draw_image(void *image, const FRect *dst, const FRect *src) = 0;

        // Fill
        virtual bool fill_path(void *path, void *brush) = 0;
        virtual bool fill_rect(FRect rect, void *brush) = 0;
        virtual bool fill_ellipse(FPoint center, float xr, float yr, void *brush) = 0;
        virtual bool fill_mask(void *mask, const FRect *dst, const FRect *src) = 0;

        /**
         * @brief Set the transform object
         * 
         * @param mat The transform matrix, nullptr on Identity matrix
         * @return true 
         * @return false 
         */
        virtual bool set_transform(const FMatrix *mat) = 0;

        /**
         * @brief Set the scissor object
         * 
         * @param rect The scissor rectangle, nullptr on no scissor
         * @return true 
         * @return false 
         */
        virtual bool set_scissor(const FRect *rect) = 0;

        virtual bool has_feature(PaintContextFeature feature) = 0;

        virtual bool native_handle(PaintNativeContext ctxt, void *result) = 0;
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

BTK_NS_END