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

class AbstractFileDialog : public Any {
    public:
        using Option = FileDialog::Option;

        virtual int        run() = 0;
        virtual bool       initialize() = 0;
        virtual void       set_dir(u8string_view dir) = 0;
        virtual void       set_title(u8string_view title) = 0;
        virtual void       set_option(Option opt) = 0;
        virtual StringList result() = 0;
};

BTK_FLAGS_OPERATOR(FileDialog::Option, uint32_t);

inline auto FileDialog::option() const -> Option {
    return opt;
}

BTK_NS_END