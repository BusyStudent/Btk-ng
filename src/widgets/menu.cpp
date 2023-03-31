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
    set_parent(nullptr);
    if (wi) {
        wi->root()->add_child(this);

        raise();
        take_focus();
    }
    else {
        show();
        raise();
    }
}

bool PopupWidget::paint_event(PaintEvent &) {
    auto &p = painter();
    if (is_window()) {
        // Popup like window
        return true;
    }

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
    _focus_lost.emit();
    if (hide_on_focus_lost) {
        hide();
    }
    else {
        close();
    }
    return true;
}
bool PopupWidget::mouse_press(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool PopupWidget::mouse_release(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}

BTK_NS_END