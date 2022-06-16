#pragma once

#include <Btk/detail/painting.hpp>
#include <Btk/rect.hpp>

BTK_NS_BEGIN

class Vertex {
    public:
        FPoint  position;
        FPoint  texcoord;
        GLColor color;
};

class BTKAPI Painter {
    public:
        Painter(Widget *widget);
        Painter(const Painter &) = delete;
        ~Painter();

        // Texture create_texture(int width, int height);

        void fill_color(GLColor color);
        void draw_line(FPoint p1, FPoint p2);
        void draw_rect(FRect rect);
        void fill_rect(FRect rect);
        void draw_text(u8string_view txt, float x, float y);
    private:
        gcontext_t _gcontext = nullptr;
};

BTK_NS_END