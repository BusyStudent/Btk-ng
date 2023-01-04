#include "build.hpp"

#include <Btk/widgets/frame.hpp>
#include <Btk/event.hpp>

// Frame

BTK_NS_BEGIN

Frame::Frame(Widget *parent) : Widget(parent) {}
Frame::~Frame() {}

bool Frame::paint_event(PaintEvent &) {
    auto &painter = this->painter();
    painter.set_stroke_width(_line_width);
    switch (_frame_style) {
        case NoFrame : break;
        case Box : {
            if (under_mouse()) {
                painter.set_color(style()->border);
            }
            else {
                painter.set_color(style()->highlight);
            }
            painter.draw_rect(rect());
            break;
        }
    }
    painter.set_stroke_width(1.0f);
    return true;
}


BTK_NS_END