#pragma once

#include <Btk/opengl/opengl_macro.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

/**
 * @brief OpenGL ES2 Function table
 * 
 */
class OpenGLES2Function {
    public:
        #define BTK_GL_PROCESS(pfn, name) \
            pfn name = nullptr;
        
        BTK_OPENGLES2_FUNCTIONS_PART

        #undef  BTK_GL_PROCESS

        #define BTK_GL_PROCESS(pfn, name) \
            name = reinterpret_cast<pfn>(callable(#name)); \
            if (!name) { \
                bad_fn(#name); \
            }

        template <typename Callable>
        void load (Callable &&callable) {
            BTK_OPENGLES2_FUNCTIONS_PART
        }

        #undef  BTK_GL_PROCESS


        // Got nullptr on load
        void bad_fn(const char *name) {

        }
};

/**
 * @brief OpenGL ES3 Function table
 * 
 */
class OpenGLES3Function : public OpenGLES2Function {
    public:
        #define BTK_GL_PROCESS(pfn, name) \
            pfn name = nullptr;
        
        BTK_OPENGLES3_FUNCTIONS_PART

        #undef  BTK_GL_PROCESS

        #define BTK_GL_PROCESS(pfn, name) \
            name = reinterpret_cast<pfn>(callable(#name)); \
            if (!name) { \
                bad_fn(#name); \
            }

        template <typename Callable>
        void load (Callable &&callable) {
            OpenGLES2Function::load(callable);
            BTK_OPENGLES3_FUNCTIONS_PART
        }

        #undef  BTK_GL_PROCESS
};

#define BTK_GL_PROCESS(pfn, name) \
    template <typename ...Args> \
    auto name(Args &&...args) BTK_NOEXCEPT_IF(this->_funcs->name(args...)) { \
        return this->_funcs->name(std::forward<Args>(args)...);\
    }

/**
 * @brief Helper class for redirecting all opengles2 functions call to dst
 * 
 * @tparam T The type of opengl functions table
 */
template <typename T = OpenGLES2Function>
class OpenGLES2FunctionProxy {
    public:
        OpenGLES2FunctionProxy(T *f = nullptr) : _funcs(f) {}

        BTK_OPENGLES2_FUNCTIONS_PART

        void set_functions(T *f) noexcept {
            _funcs = f;            
        }
    protected:
        T *_funcs = nullptr;
};
/**
 * @brief Helper class for redirecting all opengles3 functions call to dst
 * 
 * @tparam T The type of opengl functions table
 */
template <typename T = OpenGLES3Function>
class OpenGLES3FunctionProxy : public OpenGLES2FunctionProxy<T> {
    public:
        OpenGLES3FunctionProxy(T *f) : OpenGLES2FunctionProxy<T>(f) {}

        BTK_OPENGLES3_FUNCTIONS_PART
};

#undef BTK_GL_PROCESS

BTK_NS_END