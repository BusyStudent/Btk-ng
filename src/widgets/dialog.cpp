#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/widgets/button.hpp> //< For Button
#include <Btk/widgets/view.hpp> //< For label

#include <Btk/context.hpp>

BTK_NS_BEGIN

Dialog::Dialog() {

}
Dialog::~Dialog() {

}
int  Dialog::run() {
    show();

    EventLoop loop(GetUIDisplatcher());
    _closed.connect(&EventLoop::stop);
    return loop.run();
}
void Dialog::open() {
    show();

}
bool Dialog::close_event(CloseEvent &event) {
    _closed.emit();
    return false; //< Default in false, in not handled
}

FileDialog::FileDialog() {
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