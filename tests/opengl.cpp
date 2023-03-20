#include <Btk/opengl/opengl_widget.hpp>
#include <Btk/opengl/opengl.hpp>
#include <Btk/context.hpp>
#include <Btk/event.hpp>
#include <algorithm>
#include <iostream>

using namespace BTK_NAMESPACE;

#define GL_CHECK() if (glGetError() != GL_NO_ERROR) { std::cerr << "OpenGL error" << std::endl; assert(false); }

char *vertex_code = R"(
    #version 330 core

    layout (location = 0) in vec3 input_vertex;

    void main() {
        gl_Position = vec4(input_vertex.x, input_vertex.y, input_vertex.z, 1.0);
    }
)";

char *frag_code = R"(
    #version 330 core

    out vec4 color;

    void main() {
        color = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

class GLCanvas : public GLWidget, public OpenGLES3Function {
    public:
        GLCanvas() = default;
        ~GLCanvas() = default;
    private:
        void gl_ready() override;
        void gl_paint() override;

        bool drag_begin(DragEvent &) override;
        bool drag_motion(DragEvent &) override;
        bool drag_end(DragEvent &) override;

        void update_buffer();

        FPoint norm_point(FPoint p);

        GLint program = 0;
        GLuint vertex_array;
        GLuint buffer;

        FPoint p1 {-0.5f, -0.5f};
        FPoint p2 {0.5f, -0.5f};
        FPoint p3 {0.0f, 0.5f};

        FPoint *out = nullptr;

        bool ready = false;
};

void GLCanvas::gl_ready() {
    gl_load_function(this);

    GLint vert = glCreateShader(GL_VERTEX_SHADER);
    const char * vcode = vertex_code;
    glShaderSource(vert, 1, &vcode, nullptr);
    glCompileShader(vert);

    GLint frag = glCreateShader(GL_FRAGMENT_SHADER);
    const char * fcode = frag_code;
    glShaderSource(frag, 1, &fcode, nullptr);
    glCompileShader(frag);

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    // VertexArray
    glGenVertexArrays(1, &vertex_array);
    GL_CHECK();
    glBindVertexArray(vertex_array);
    GL_CHECK();

    glGenBuffers(1, &buffer);
    GL_CHECK();
    
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GL_CHECK();

    GLfloat vertex_data[] = {
        // -1.0f, -1.0f, 0.0f,
        //  1.0f,  1.0f, 0.0f,
        //  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    GL_CHECK();

    // Tell vertex info
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
    GL_CHECK();

    // Put the array to location 0
    glEnableVertexAttribArray(0);
    GL_CHECK();

    GLuint line_array;
    glGenVertexArrays(1, &line_array);
    glBindVertexArray(line_array);

    GLuint line_buffer;
    glGenBuffers(1, &line_buffer);

    GLfloat line_vertex [] = {
        1.0, 1.0,
        1.0, 1.0,
        1.0, 1.0,
        1.0, 1.0,
    };

    std::cout << "OpenGL Version : " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "OpenGL Vendor : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer : " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Extensions : " << glGetString(GL_EXTENSIONS) << std::endl;

    ready = true;

}
void GLCanvas::gl_paint() {
    glClearColor(0.1, 0.2, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    GL_CHECK();
    glBindVertexArray(vertex_array);
    GL_CHECK();
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
bool GLCanvas::drag_motion(DragEvent &event) {
    if (!ready) {
        return true;
    }
    *out = norm_point(event.position());
    update_buffer();
    return true;
}
bool GLCanvas::drag_begin(DragEvent &event) {
    if (!ready) {
        return true;
    }
    auto p = norm_point(event.position());
    auto lp1 = (p1 - p).length();
    auto lp2 = (p2 - p).length();
    auto lp3 = (p3 - p).length();
    auto m = std::min({lp1, lp2, lp3});

    if (m == lp1) {
        out = &p1;
    }
    if (m == lp2) {
        out = &p2;
    }
    if (m == lp3) {
        out = &p3;
    }

    return true;
}
bool GLCanvas::drag_end(DragEvent &event) {
    return true;
}
void GLCanvas::update_buffer() {
    gl_make_current();
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GLfloat vertex_data[] = {
        // -1.0f, -1.0f, 0.0f,
        //  1.0f,  1.0f, 0.0f,
        //  0.0f,  1.0f, 0.0f,
         p1.x,  p1.y, 0.0f, // left  
         p2.x,  p2.y, 0.0f, // right 
         p3.x,  p3.y, 0.0f  // top   
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    repaint();
}

FPoint GLCanvas::norm_point(FPoint pos) {
    // Normalize it
    auto r = rect().cast<float>();
    pos.x = ((pos.x - r.x) - (r.w / 2)) / (r.w / 2);
    pos.y = ((pos.y - r.y) - (r.h / 2)) / (r.h / 2);
    return pos;
}

int main () {
    UIContext ctxt;
    GLCanvas canvas;
    canvas.set_window_title("Btk OpenGL Canvas");
    canvas.resize(640, 740);
    canvas.show();
    

    return ctxt.run();
}