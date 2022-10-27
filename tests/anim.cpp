#include <Btk/plugins/animation.hpp>
#include <Btk/context.hpp>
#include <Btk/comctl.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

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

    ctxt.run();
}