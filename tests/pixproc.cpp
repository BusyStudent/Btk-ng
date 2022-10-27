#include <Btk/context.hpp>
#include <Btk/comctl.hpp>

using namespace BTK_NAMESPACE;

class Test : public Widget {
    public:
        Test() {
            // Kernel 3x3
            global.attach(this);

            auto ctrl = new BoxLayout(TopToBottom);
            global.add_layout(ctrl);
            global.add_widget(&view, 1);

            auto lay = new BoxLayout(TopToBottom);
            auto btn = new Button("Filter2D");
            ctrl->add_layout(lay, 1);
            ctrl->add_stretch(2);
            ctrl->add_widget(btn, 0, Alignment::Center | Alignment::Middle);

            btn->signal_clicked().connect([this]() {
                view.set_keep_aspect_ratio(true);
                view.set_image(source.filter2d(kernel));
            });

            for (int y = 0;y < 3; y ++) {
                auto sub = new BoxLayout(LeftToRight);
                lay->add_layout(sub);
                for (int x = 0; x < 3; x++) {
                    auto edit = new TextEdit("0.0");
                    sub->add_widget(edit);

                    edit->signal_text_changed().connect([=]() {
                        float value = 0;
                        if (sscanf(edit->text().data(), "%f", &value)) {
                            kernel[y][x] = value;

                            // Update kernel
                            printf("Update kernel \n");

                            for (int y = 0;y < 3; y ++) {
                                for (int x = 0; x < 3; x++) {
                                    printf ("kernel [%d, %d] = %f\n", x, y, float(kernel[y][x]));
                                }
                            }
                        }
                    });
                }
            }

            view.set_image(source);
        }

        void param_changed() {

        }
    public:
        PixBuffer source = PixBuffer::FromFile("icon.jpeg");
        ImageView view;
        BoxLayout global{LeftToRight};
        double kernel[3][3] = {0.0};
};
class BlurTest : public Widget {
    public:
        BlurTest() {
            buffer = PixBuffer::FromFile("icon.jpeg");
            label.set_text("Blur 0px");

            layout.set_direction(TopToBottom);
            layout.attach(this);
            layout.add_widget(&label, 0, Alignment::Left | Alignment::Middle);
            layout.add_widget(&slider, 0, Alignment::Left | Alignment::Middle);
            layout.add_widget(&display, 1);

            display.set_keep_aspect_ratio(true);
            display.set_image(buffer);

            slider.signal_value_changed().connect(&BlurTest::value_changed, this);
        }

        void value_changed() {
            label.set_text(u8string::format("Blur %dpx", int(slider.value())));
            display.set_image(buffer.blur(slider.value()));
        }
    private:
        Label     label;
        BoxLayout layout;
        ImageView display;
        Slider    slider;
        PixBuffer buffer;
};


int main() {
    Btk::UIContext ctxt;
    Test test;
    test.resize(300, 200);
    test.show();

    BlurTest blur;
    blur.resize(600, 800);
    blur.show();

    ctxt.run();
}