#include "build.hpp"

#include <Btk/widgets/container.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

StackedWidget::StackedWidget(Widget *parent) : Widget(parent) { }
StackedWidget::~StackedWidget() { }

void StackedWidget::add_widget(Widget *w) {
    if (!w) {
        return;
    }
    w->hide();
    _widgets.push_back(w);
    add_child(w);
}
void StackedWidget::insert_widget(int idx, Widget *w) {
    if (!w) {
        return;
    }
    if (idx < 0) {
        return;
    }
    if (idx >= _widgets.size()) {
        idx = _widgets.size();
    }
    _widgets.insert(_widgets.begin() + idx, w);
    add_child(w);
    w->hide();
}
void StackedWidget::remove_widget(Widget *w) {
    auto iter = std::find(_widgets.begin(), _widgets.end(), w);
    if (iter != _widgets.end()) {
        if (*iter == current_widget()) {
            set_current_widget(nullptr);
        }
        _widgets.erase(iter);
        remove_child(w);
    }
}
void StackedWidget::delete_widget(Widget *w) {
    auto iter = std::find(_widgets.begin(), _widgets.end(), w);
    if (iter != _widgets.end()) {
        if (*iter == current_widget()) {
            set_current_widget(nullptr);
        }
        _widgets.erase(iter);
        remove_child(w);

        delete w;
    }
}
bool StackedWidget::resize_event(ResizeEvent &event) {
    auto w = current_widget();
    if (w) {
        w->resize(event.width(), event.height());
    }
    return true;
}
bool StackedWidget::move_event(MoveEvent &event) {
    auto w = current_widget();
    if (w) {
        w->move(event.x(), event.y());
    }
    return true;
}
Widget *StackedWidget::current_widget() const {
    if (_current < 0 || _current >= _widgets.size()) {
        // Invalid
        return nullptr;
    }
    return _widgets[_current];
}
int     StackedWidget::current_index() const {
    return _current;
}
int     StackedWidget::index_of(Widget *w) const {
    if (!w) {
        return -1;
    }
    auto iter = std::find(_widgets.begin(), _widgets.end(), w);
    if (iter == _widgets.end()) {
        return -1;
    }
    return iter - _widgets.begin();
}
void    StackedWidget::set_current_widget(Widget *w) {
    set_current_index(index_of(w));
}
void   StackedWidget::set_current_index(int idx) {
    auto cur = current_widget();
    if (cur) {
        cur->hide();
    }
    _current = idx;
    cur = current_widget();
    if (cur) {
        auto r = rect();
        cur->show();
        cur->move(r.x, r.y);
        cur->resize(r.w, r.h);
    }

    _current_changed.emit(_current);
}

BTK_NS_END