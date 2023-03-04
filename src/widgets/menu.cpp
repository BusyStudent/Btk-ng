#include "build.hpp"

#include <Btk/widgets/menu.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

Action::Action() {}
Action::~Action() {}

PopupWidget::PopupWidget() {
    set_focus_policy(FocusPolicy::Mouse);
}
PopupWidget::~PopupWidget() {

}
void PopupWidget::popup(Widget *wi) {
    if (wi) {
        wi->root()->add_child(this);

        raise();
        take_focus();
    }
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
bool PopupWidget::focus_lost(FocusEvent &) {
    close();
    return true;
}

BTK_NS_END