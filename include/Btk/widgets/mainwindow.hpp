#pragma once

#include <Btk/widgets/menu.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

class BTKAPI MainWindow : public Widget {
    public:
        MainWindow(Widget *parent = nullptr);
        ~MainWindow();

        bool handle(Event &) override;
        void set_widget(Widget *wi);

        MenuBar &menubar() noexcept {
            return _menubar;
        }
        Widget *widget() const noexcept {
            return _widget;
        }
    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
    private:
        void relayout();

        MenuBar _menubar {this};
        Widget *_widget = nullptr; // Main window widget. There can be only one.
        bool    _except_relayout = false;
};

BTK_NS_END