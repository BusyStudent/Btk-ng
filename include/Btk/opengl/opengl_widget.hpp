#pragma once 

#include <Btk/opengl/opengl.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

class GLWidgetImpl;
class GLContext;
/**
 * @brief Widget for OpenGL rendering
 * 
 */
class BTKAPI GLWidget : public Widget {
    public:
        GLWidget();
        ~GLWidget();

        GLContext *gl_context();
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
    protected:
        bool  paint_event(PaintEvent &event) override;
        bool  resize_event(ResizeEvent &event) override;
        bool  move_event(MoveEvent &event) override;
    private:
        bool  gl_initialize();

        GLWidgetImpl *priv = nullptr;
        bool          init_failed = false;
};

BTK_NS_END