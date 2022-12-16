#pragma once 

#include <Btk/opengl/opengl.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

/**
 * @brief Widget for OpenGL rendering
 * 
 */
class BTKAPI GLWidget : public Widget {
    public:
        GLWidget();
        ~GLWidget();

        /**
         * @brief Get the OpenGL Proc Address (only valid after gl_ready was called)
         * 
         * @param proc_name 
         * @return void* 
         */
        void *gl_get_proc_address(const char_t *proc_name);
        /**
         * @brief Get the OpenGL Drawable size (only valid after gl_ready was called)
         * 
         * @return Size The pixel size of the drawable
         */
        Size  gl_drawable_size();
        /**
         * @brief Make the OpenGL to current state (only valid after gl_ready was called
         * 
         */
        bool  gl_make_current();

        template <typename T>
        void  gl_load_function(T *funcs) {
            funcs->load([this](const char_t * proc_name) {
                return gl_get_proc_address(proc_name);
            });
        }

        bool  paint_event(PaintEvent &event) override;
        bool  resize_event(ResizeEvent &event) override;

        /**
         * @brief Notify the user that OpenGL is ready
         * 
         */
        virtual void gl_ready() {};
        /**
         * @brief Notify the user Show Paint the OpenGL
         * 
         */
        virtual void gl_paint() = 0;
    private:
        void     gl_initialize();
        Texture  texture = {};    //< Painter's texture for drawing the opengl framebuffer 
        window_t glwin = nullptr; //< Physical window never showed used for rendering        
        any_t    glctxt = nullptr; //< OpenGL context layer

        // Buffers 
        void   *buffer      = nullptr; //< Raw pixel of this framebuffer
        Size    buffer_size = {}; //< Size of the pixel buffer

        // OpenGL functions
        void    *Viewport   = nullptr;
        void    *ReadPixels = nullptr;
        void    *GetError   = nullptr;
};

BTK_NS_END