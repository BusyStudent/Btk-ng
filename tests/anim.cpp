#include <Btk/plugins/animation.hpp>
#include <Btk/context.hpp>
#include <Btk/comctl.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

class ColorRect : public Widget {
    public:
        ColorRect() {
            LinearGradient lg;
            lg.add_stop(0, Color::Red);
            lg.add_stop(0.5, Color::Green);
            lg.add_stop(1, Color::Blue);

            gradient.set_gradient(lg);
        }
        bool paint_event(PaintEvent &) override {
            painter().set_brush(gradient);
            painter().fill_rect(rect());

            return true;
        }
    private:
        Brush gradient;
};

int main() {
    UIContext ctxt;
    Frame  widget;
    Button btn(&widget);

    LerpAnimation<Size> s;
    s.set_start_value(Size(200, 200));
    s.set_end_value(Size(400, 500));
    s.add_key_value(0.2f, Size(100, 100));
    s.add_key_value(0.8f, Size(600, 600));


    s.bind(&Widget::resize, &btn);
    s.set_duration(2000);

    widget.show();
    widget.resize(500, 600);
    btn.signal_clicked().connect([&]() {
        s.start();
    });

    ScrollArea area;
    ColorRect lg;
    area.show();
    
    lg.set_rect(0, 0, 1000, 1000);
    area.set_viewport(&lg);

    ctxt.run();
}