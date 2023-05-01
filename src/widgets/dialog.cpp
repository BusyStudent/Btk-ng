#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/widgets/button.hpp> //< For Button
#include <Btk/widgets/view.hpp> //< For label

#include <Btk/context.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

Dialog::Dialog(Widget *parent) : Widget(parent) {
    
}
Dialog::~Dialog() {

}
int  Dialog::run() {
    internal_show();

    EventLoop loop(GetUIDisplatcher());
    _closed.connect(&EventLoop::stop, &loop);
    return loop.run();
}
void Dialog::open() {
    internal_show();

}
void Dialog::internal_show() {
    show();

    // Remove prev filter

    if (is_root()) {
        // As popup
        return;
    }
    if (size() == Size(0, 0)) {
        // No size
        resize(size_hint());
    }

    // Move to current place
    move(Rect(0, 0, parent()->size()).align_object(size(), AlignMiddle | AlignCenter).position());
    // Inject filter
    parent()->add_event_filter(Filter, this);
}

bool Dialog::close_event(CloseEvent &event) {
    _closed.emit();
    return false; //< Default in false, in not handled
}

bool Dialog::paint_event(PaintEvent &event) {
    if (is_root()) {
        // Popup, nothing to do
        return true;
    }
    auto &p = painter();

    // Draw shadow
    p.save();
    p.set_color(Color::Gray);
    p.set_alpha(p.alpha() * 0.8f);
    p.fill_rect(parent()->rect());
    p.restore();

    p.set_brush(palette().window());
    p.fill_rect(rect());

    // Draw self....
    return true;
}
bool Dialog::mouse_press(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool Dialog::mouse_release(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool Dialog::mouse_motion(MotionEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool Dialog::mouse_wheel(WheelEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool Dialog::resize_event(ResizeEvent &) {
    // Replace
    if (parent()) {
        move(Rect(0, 0, parent()->size()).align_object(size(), AlignMiddle | AlignCenter).position());
    }
    return true;
}

bool Dialog::Filter(Object *object, Event &event, void *_self) {
    Dialog *self = static_cast<Dialog*>(_self);
    switch (event.type()) {
        // case Event::MouseMotion : {
        //     if (!self->rect().contains(event.as<MotionEvent>().position())) {
        //         // Out of range, Drop
        //         return FilterResult::Discard;
        //     }
        //     break;
        // }
        // case Event::MouseRelease : {
        //     [[fallthrough]];
        // }
        // case Event::MousePress : {
        //     if 	(!self->rect().contains(event.as<MouseEvent>().position())) {
        //         return FilterResult::Discard;
        //     }
        //     break;
        // }
        case Event::Resized : {
            Rect rect;
            rect.x = 0;
            rect.y = 0;
            rect.w = event.as<ResizeEvent>().width();
            rect.h = event.as<ResizeEvent>().height();

            self->move(rect.align_object(self->size(), AlignMiddle | AlignCenter).position());
            break;
        }
        // case Event::Moved : {
        //     auto rect = object->as<Widget*>()->rect();
        //     rect.x = event.as<MoveEvent>().x();
        //     rect.y = event.as<MoveEvent>().y();

        //     self->move(rect.align_object(self->size(), AlignMiddle | AlignCenter).position());
        //     break;
        // }
    }
    return FilterResult::Keep;
}

FileDialog::FileDialog(Widget *parent) : Dialog(parent) {
    auto ret = driver()->instance_create("filedialog");
    if (ret) {
        native = ret->as<AbstractFileDialog*>();
    }
}
FileDialog::~FileDialog() {
    delete native;
}
int FileDialog::run() {
    assert(native);
    native->initialize();
    return native->run();
}
void FileDialog::open() {
    // Currently unsupport async
    run();
}
StringList FileDialog::result() const {
    return native->result();
}
void       FileDialog::set_option(Option o) {
    opt = o;
    if (native) {
        native->set_option(opt);
    }
}



BTK_NS_END