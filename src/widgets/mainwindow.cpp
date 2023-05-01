#include "build.hpp"

#include <Btk/widgets/mainwindow.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

MainWindow::MainWindow(Widget *parent) : Widget(parent) { }
MainWindow::~MainWindow() { }

bool MainWindow::handle(Event &event) {
    if (Widget::handle(event))  {
        return true;
    }
    if (event.type() == Event::LayoutRequest && !_except_relayout) {
        relayout();
        return true;
    }
    return false;
}
void MainWindow::set_widget(Widget *wi) {
    if (_widget) {
        _widget->set_parent(nullptr);
    }
    _widget = wi;
    if (_widget) {
        _widget->set_parent(this);
    }
    relayout();
}
void MainWindow::relayout() {
    _except_relayout = true;
    if (_menubar.count_items() == 0) {
        _menubar.hide();
        if (_widget) {
            _widget->set_rect(FRect(0, 0, size()));
        }
    }
    else {
        _menubar.move(0, 0);
        _menubar.resize(width(), _menubar.size_hint().h);
        _menubar.show();

        if (_widget) {
            _widget->move(_menubar.rect().bottom_left());
            _widget->resize(width(), height() - _menubar.height());
        }
    }
    _except_relayout = false;
}
bool MainWindow::resize_event(ResizeEvent &event) {
    relayout();
    return true;
}
bool MainWindow::move_event(MoveEvent &evemt) {
    relayout();
    return true;
}

BTK_NS_END