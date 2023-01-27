#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

using namespace BTK_NAMESPACE;

class PaintBox: public Widget {
    public:
        PaintBox() {
            auto f = font();
            f.set_size(60);
            layout.set_text("Hello World");
            layout.set_font(f);

            path = layout.outline();
        }

        bool paint_event(PaintEvent &) override;
        bool drag_motion(DragEvent &event) override;
        bool drag_begin(DragEvent &event) override;
        bool drag_end(DragEvent &event) override;

        bool key_press(KeyEvent &event) override;
    private:
        std::vector<std::vector<FPoint>> points;
        TextLayout layout;
        PainterPath path;
};

int main() {
    UIContext ctxt;
    PaintBox box;
    box.show();

    box.resize(1000, 1000);

    return ctxt.run();
}

bool PaintBox::drag_motion(DragEvent &event) {
    points.back().push_back(event.position());
    repaint();
    return true;
}
bool PaintBox::drag_begin(DragEvent &event) {
    points.emplace_back();
    return true;
}
bool PaintBox::drag_end(DragEvent &event) {
    if (points.back().size() < 2) {
        points.pop_back();
    }
    return true;
}
bool PaintBox::paint_event(PaintEvent &event) {
    auto &p = painter();
    p.set_color(Color::Black);

    for (auto &vec : points) {
        if (vec.size() < 2) {
            continue;
        }
        for (int i = 0; i < vec.size() - 1; i++) {
            p.draw_line(vec[i], vec[i+1]);
        }
    }

    return true;
}

bool PaintBox::key_press(KeyEvent &event) {
    switch (event.key()) {
        case Key::Escape : {
            points.clear();
            repaint();
            break;
        }
        default : return false;
    }
    return true;
}