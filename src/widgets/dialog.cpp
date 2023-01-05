#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/context.hpp>

BTK_NS_BEGIN

FileDialog::FileDialog() {
    auto ret = driver()->instance_create("filedialog");
    if (ret) {
        assert(BTK_CASTABLE(AbstractFileDialog, ret));
        native = static_cast<AbstractFileDialog*>(ret);
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