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
    auto name(Args &&...args) { \
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
        OpenGLES2FunctionProxy(const OpenGLES2FunctionProxy &) = default;

        BTK_OPENGLES2_FUNCTIONS_PART

        void set_function(T *f) noexcept {
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
        OpenGLES3FunctionProxy(T *f = nullptr) : OpenGLES2FunctionProxy<T>(f) {}
        OpenGLES3FunctionProxy(const OpenGLES3FunctionProxy &) = default;

        BTK_OPENGLES3_FUNCTIONS_PART
};

/**
 * @brief OpenGL Shader wrapper
 * 
 * @tparam T The OpenGL proxy
 */
template <typename T = OpenGLES3FunctionProxy<>> 
class OpenGLShader : public T {
    public:
        static constexpr GLenum Vertex   = GL_VERTEX_SHADER;
        static constexpr GLenum Fragment = GL_FRAGMENT_SHADER;

        /**
         * @brief Construct a new Open GL Shader object
         * 
         * @param type Shader type (Vertex or Fragment)
         */
        OpenGLShader(GLenum type) { _shader = glCreateShader(type); }
        /**
         * @brief Construct a new Open G L Shader object
         * 
         * @param base The args pass to Base class constructor
         * @param type Shader type (Vertex or Fragment)
         */
        OpenGLShader(const T &base, GLenum type) : T(base), OpenGLShader(type) {}
        OpenGLShader(OpenGLShader &&s) { _shader = s.detach(); }
        OpenGLShader(const OpenGLShader &) = delete;
        ~OpenGLShader() { clear(); }

        GLuint get() const noexcept {
            return _shader;
        }

        GLuint detach() {
            auto s = _shader;
            _shader = 0;
            return s;
        }

        void   attach(GLuint shader) {
            clear();
            _shader = shader;
        }
        void   clear() {
            if (_shader != 0) {
                glDeleteShader(_shader);
                _shader = 0;
            }
        }
        bool    compile(const char *str, GLint len) {
            glShaderSource(_shader, 1, &str, &len);
            glCompileShader(_shader);

            GLint status;
            glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
            return status == GL_TRUE;
        }
        bool    compile(const char *str) {
            return compile(str, ::strlen(str));
        }
    private:
        GLuint _shader = 0;
};

/**
 * @brief OpenGL Program wrapper
 * 
 */
template <typename T = OpenGLES3FunctionProxy<>>
class OpenGLProgram : public T {
    public:
        OpenGLProgram() = default;
        OpenGLProgram(const T &p) : T(p) {}
        OpenGLProgram(OpenGLProgram &other) { _program = other.detach(); }
        OpenGLProgram(const OpenGLProgram &) = delete;
        ~OpenGLProgram() { clear(); }

        template <typename Shader>
        bool link_with(Shader &vertex, Shader &fragment) {
            if (_program == 0) {
                _program = glCreateProgram();
            }
            glAttachShader(_program, vertex.get());
            glAttachShader(_program, fragment.get());
            glLinkProgram(_program);

            GLint status;
            glGetProgramiv(_program, GL_LINK_STATUS, &status);
            return status == GL_TRUE;
        }
        void use() {
            glGetIntegerv(GL_PROGRAM, &prev_program);
            glUseProgram(_program);
        }
        void unuse() {
            glUseProgram(prev_program);
        }
        void clear() {
            if (_program != 0) {
                glDeleteProgram(_program);
                _program = 0;
            }
        }

        GLuint get() const noexcept {
            return _program;
        }
        GLuint detach() {
            auto s = _program;
            _program = 0;
            return s;
        }
    private:
        GLint prev_program = 0;
        GLuint    _program = 0;
};

#undef BTK_GL_PROCESS

BTK_NS_END