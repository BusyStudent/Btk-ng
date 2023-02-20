#include "build.hpp"

#include <Btk/widgets/menu.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

Action::Action() {}
Action::~Action() {}

PopupWidget::PopupWidget() {}
PopupWidget::~PopupWidget() {
    if (attached_widget) {
        attached_widget->del_event_filter(Filter, this);
    }
}
void PopupWidget::popup(Widget *wi) {
    if (wi) {
        wi->root()->add_child(this);
        wi->add_event_filter(Filter, this);

        attached_widget = wi;

        raise();
    }
}

bool PopupWidget::Filter(Object *, Event &event, void *_self) {
    // Root window mouse pressed
    if (event.type() == Event::MousePress) {
        auto self = static_cast<PopupWidget*>(_self);
        if (!self->rect().contains(event.as<MouseEvent>().position())) {
            self->close();
        }
    }
    return false;
}
bool PopupWidget::paint_event(PaintEvent &) {
    auto &p = painter();

    // Draw shadow
    p.save();
    p.translate(2, 2);
    p.set_color(Color::Gray);
    p.fill_rect(rect());
    p.restore();

    p.set_brush(palette().window());
    p.fill_rect(rect());
    p.set_brush(palette().border());
    p.draw_rect(rect());

    return true;
}

BTK_NS_END