#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class AbstractFileDialog;

class BTKAPI  FileDialog : public Widget {
    public:
        enum Option : uint32_t {
            Open  = 0,
            Save  = 1 << 0,
            Multi = 1 << 1, 
        };

        FileDialog();
        ~FileDialog();

        int        run();
        Option     option() const;
        StringList result() const;

        void set_option(Option opt);
    private:
        AbstractFileDialog *native;
        Option              opt = {};
};

BTK_FLAGS_OPERATOR(FileDialog::Option, uint32_t);

inline auto FileDialog::option() const -> Option {
    return opt;
}
inline void FileDialog::set_option(Option o) {
    opt = o;
}


BTK_NS_END