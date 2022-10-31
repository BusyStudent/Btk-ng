#include "build.hpp"

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
    native->initialize(!uint32_t(opt & Open));
    return native->run();
}
StringList FileDialog::result() const {
    return native->result();
}

BTK_NS_END