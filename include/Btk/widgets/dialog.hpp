#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

class AbstractFileDialog;

class BTKAPI Dialog : public Widget {
    public:
        Dialog(Widget *parent = nullptr);
        ~Dialog();

        virtual int  run();
        virtual void open();
    protected:
        bool close_event(CloseEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_wheel(WheelEvent &event) override;
        bool mouse_motion(MotionEvent &event) override;
        bool resize_event(ResizeEvent &event) override;
    private:
        static bool Filter(Object *, Event &, void *);
        void internal_show();
        void on_filter_object_destroyed();

        Signal<void()> _closed;
};

class BTKAPI FileDialog : public Dialog {
    public:
        enum Option : uint32_t {
            Open  = 0,
            Save  = 1 << 0,
            Multi = 1 << 1, 
        };

        FileDialog(Widget *parent = nullptr);
        ~FileDialog();

        int        run() override;
        void       open() override;
        Option     option() const;
        StringList result() const;

        void set_option(Option opt);
    private:
        AbstractFileDialog *native;
        Option              opt = {};
};

// class BTKAPI MessageBox : public Dialog {
//     public:
//         MessageBox(Widget *parent = nullptr);
//         ~MessageBox();

//         void set_title(u8string_view title);
//         void set_message(u8string_view message);
//     private:
//         u8string _title;
//         u8string _message;
// };

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