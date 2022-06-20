#include <Btk/context.hpp>
#include <Btk/widget.hpp>

using namespace Btk;

class Canvas : public Widget {
    public:
        bool paint_event(PaintEvent &e) override {
            auto gc = this->gc();
            gc->set_color(0, 0, 0);

            if (x == -1) {
                x = this->width();
            }
            if (y == -1) {
                y = this->height();
            }

            FPoint points [] = {
                FPoint(0, 0),
                FPoint(x, y)
            };

            // printf("Canvas::paint_event: %d %d\n", this->width(), this->height());

            gc->draw_lines(points,2);

            return true;
        }
        bool mouse_motion(MotionEvent &e) override {
            x = e.x();
            y = e.y();
            repaint();
            return true;
        }

        int x = -1;
        int y = -1;
};

int main () {
    for(uchar_t ch : u8string_view("HelloWorld, 你好世界")) {
        printf("%d\n", ch);
    }

    auto device = SDLDriverInfo.create();
    UIContext context(device);

    Button w;
    w.show();
    w.set_flat(true);
    // Canvas c;
    // c.show();


    context.run();
}