#include <Btk/plugins/animation.hpp>
#include <Btk/widgets/container.hpp>
#include <Btk/widgets/textedit.hpp>
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
    Button btn;

    LerpAnimation<Size> s;
    s.set_start_value(Size(200, 200));
    s.set_end_value(Size(400, 500));
    s.add_key_value(0.2f, Size(100, 100));
    s.add_key_value(0.8f, Size(600, 600));


    // s.bind(&Widget::resize, &btn);
    s.bind([&](Size p) {
        btn.set_window_position(p.w, p.h);
        btn.resize(p.w, p.h);
    });
    s.set_duration(2000);

    btn.show();
    btn.resize(500, 600);
    btn.signal_clicked().connect([&]() {
        s.start();
    });

    Widget op_test;
    Button op_btn(&op_test);

    op_btn.set_text("Test Op");

    LerpAnimation<float> anim;
    anim.set_start_value(1.0f);
    anim.set_end_value(1.0f);

    anim.add_key_value(0.2f, 0.1f);
    anim.add_key_value(0.5f, 0.4f);
    anim.add_key_value(0.8f, 0.8f);

    anim.set_duration(5000);
    anim.bind(&Widget::set_opacity, &op_test);

    op_btn.signal_clicked().connect([&]() {
        anim.start();
    });

    op_test.show();

    TabWidget tb;

    tb.add_tab(new Button("First"), "First");
    tb.add_tab(new Button("Second"), "Second"); // Will be selected by default.
    tb.add_tab(new Button("Third"), "Third"); // Will NOT be selected by default.

    auto ctmslider = new Slider(Horizontal);
    ctmslider->signal_value_changed().connect([&]() {
        tb.set_content_margin(ctmslider->value());
    });
    tb.add_tab(ctmslider, "Slider");

    auto sw = new Button("Hide the bar");
    sw->signal_clicked().connect([&, show = true]() mutable {
        show = !show;
        tb.set_bar_visible(show);
    });

    tb.add_tab(sw, "Hide / Show test");

    // tb.set_attribute(WidgetAttrs::Debug, true);
    tb.show();

    auto subtab = new TabWidget();
    auto imageview = new ImageView();
    auto tedit = new TextEdit();
    tb.add_tab(subtab, "SubTabWidget");
    subtab->add_tab(imageview,  "ImageView");
    subtab->add_tab(tedit, "Input");

    tedit->signal_enter_pressed().connect([&]() {
        imageview->set_image(Image::FromFile(tedit->text()));
    });

    ctxt.run();
}