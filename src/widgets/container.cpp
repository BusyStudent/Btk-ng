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
    _widgets.push_back(w);
    add_child(w);
    w->hide();
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

        _except_remove = true;
        remove_child(w);
        _except_remove = false;
    }
}
void StackedWidget::delete_widget(Widget *w) {
    auto iter = std::find(_widgets.begin(), _widgets.end(), w);
    if (iter != _widgets.end()) {
        if (*iter == current_widget()) {
            set_current_widget(nullptr);
        }
        _widgets.erase(iter);

        _except_remove = true;
        remove_child(w);
        _except_remove = false;

        delete w;
    }
}
bool StackedWidget::resize_event(ResizeEvent &event) {
    auto w = current_widget();
    if (w) {
        // w->resize(event.width(), event.height());
        w->set_rect(rect().apply_margin(_margin));
    }
    return true;
}
bool StackedWidget::move_event(MoveEvent &event) {
    auto w = current_widget();
    if (w) {
        // w->move(event.x(), event.y());
        w->set_rect(rect().apply_margin(_margin));
    }
    return true;
}
bool StackedWidget::change_event(ChangeEvent &event) {
    if (event.type() == Event::ChildRemoved) {
        // A child is removed, check we cotain it
        if (_except_remove) {
            return true;
        }
        auto iter = std::find(_widgets.begin(), _widgets.end(), event.as<ChildEvent>().child());
        if (iter != _widgets.end()) {
            _widgets.erase(iter);
        }
        return true;
    }
    return false;
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
Margin  StackedWidget::margin() const {
    return _margin;
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
Widget *StackedWidget::widget_of(int idx) const {
    if (idx < 0 || idx >= int(_widgets.size())) {
        return nullptr;
    }
    return _widgets[idx];
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
        auto r = rect().apply_margin(_margin);
        cur->show();
        cur->set_rect(r);
    }

    _current_changed.emit(_current);
}
void  StackedWidget::set_margin(const Margin &m) {
    _margin = m;
    apply_margin();
}
void  StackedWidget::apply_margin() {
    auto w = current_widget();
    if (w) {
        // w->move(event.x(), event.y());
        w->set_rect(rect().apply_margin(_margin));
    }
}

// TabBar
TabBar::TabBar(Widget *parent) : Widget(parent) {

}
TabBar::~TabBar() {

}

void TabBar::add_tab(const PixBuffer &icon, u8string_view name) {
    TabItem item;
    item.text.set_text(name);
    item.text.set_font(font());
    item.icon = icon;

    _tabs.emplace_back(std::move(item));

    repaint();
}
void TabBar::insert_tab(int idx, const PixBuffer &icon, u8string_view name) {
    if (idx < 0) {
        return;
    }
    if (idx >= _tabs.size()) {
        idx = _tabs.size();
    }

    TabItem item;
    item.text.set_text(name);
    item.text.set_font(font());
    item.icon = icon;

    _tabs.insert(_tabs.begin() + idx, std::move(item));

    repaint();
}
void TabBar::remove_tab(int idx) {
    if (idx < 0) {
        return;
    }
    if (idx >= _tabs.size()) {
        return;
    }
    if (idx == _current_tab) {
        set_current_index(-1);
    }

    _tabs.erase(_tabs.begin() + idx);

    repaint();
}
void TabBar::set_tab_text(int idx, u8string_view text) {
    if (idx < 0 || idx >= _tabs.size()) {
        return;
    }
    _tabs[idx].text.set_text(text);
    repaint();
}
void TabBar::set_tab_icon(int idx, const PixBuffer &icon) {
    if (idx < 0 || idx >= _tabs.size()) {
        return;
    }
    _tabs[idx].icon.set_image(icon);
    repaint();
}
void TabBar::set_current_index(int idx) {
    if (idx >= _tabs.size()) {
        idx = -1;
    }
    _current_tab = idx;
    _current_changed.emit(idx);
    repaint();
}
bool TabBar::resize_event(ResizeEvent &) {
    return true;
}
bool TabBar::paint_event(PaintEvent &) {
    auto &p = painter();
    p.save();
    p.scissor(rect());
    for (auto &tab : _tabs) {
        int n = &tab - _tabs.data();
        auto r = tab_rect(n).cast<float>();

        if (n == _current_tab) {
            p.set_brush(palette().input());
        }
        else if (n == _hovered_tab) {
            p.set_brush(palette().light());
        }
        else {
            p.set_brush(palette().button());
        }
        p.fill_rect(r);

        p.set_brush(palette().border());
        if (n == _current_tab) {
            // We draw this border, but we didnot draw the underline
            p.draw_line(r.top_left(), r.bottom_left());
            p.draw_line(r.top_left(), r.top_right());
            p.draw_line(r.top_right(), r.bottom_right());
        }
        else {
            p.draw_rect(r);
        }

        // For text
        auto [cx, cy] = r.center();
        p.set_text_align(AlignMiddle | AlignCenter);
        p.set_brush(palette().text());
        p.draw_text(tab.text, cx, cy);
    }
    p.restore();
    return true;
}
bool TabBar::mouse_press(MouseEvent &event) {
    auto [x, y] = event.position();
    if (_tabs.size() > 0) {
        if (x > tab_rect(_tabs.size() - 1).top_right().x) {
            // <Tab, Tab, Tab> Out of range
            return true;
        }
    }
    auto new_current = tab_at(x, y);
    if (new_current != _current_tab) {
        set_current_index(new_current);
        repaint();
    }
    if (new_current != -1) {
        if (event.clicks() % 2) {
            _tab_double_clicked.emit(new_current);
        }
        else {
            _tab_clicked.emit(new_current);
        }
    }

    return true;
}
bool TabBar::mouse_motion(MotionEvent &event) {
    auto [x, y] = event.position();
    auto new_hovered = tab_at(x, y);
    if (new_hovered != _hovered_tab) {
        _hovered_tab = new_hovered;
        repaint();
    }
    return true;
}
Rect TabBar::tab_rect(int idx) const {
    if (idx < 0 || idx > int(_tabs.size())) {
        return Rect(0, 0, 0, 0);
    }
    auto margin = style()->margin;

    auto [x, y, bw, bh] = rect();
    int cur_x = x + margin;
    for (int cur = 0; cur < idx; cur++) {
        // Add margin for it
        cur_x += _tabs[cur].text.size().w + 20;
    }
    // Arrive it
    if (idx != _current_tab) {
        margin *= 2;
    }
    Rect ibr = Rect(cur_x, y + margin, _tabs[idx].text.size().w + 20, bh - margin); 	// the rect of
    return ibr;
}
int  TabBar::tab_at(int x, int y) const {
    for (auto &tab : _tabs) {
        int n = &tab - _tabs.data();
        if (tab_rect(n).contains(x, y)) {
            return n;
        }
    }
    return -1;
}
int  TabBar::count_tabs() const {
    return _tabs.size();
}
Size TabBar::size_hint() const {
    auto s = style();
    return Size(s->button_width, s->button_height);
}

// TabWidget
TabWidget::TabWidget(Widget *parent) : Widget(parent) {
    bar.signal_current_changed().connect(
        &StackedWidget::set_current_index, &display
    );
}
TabWidget::~TabWidget() {

}
void TabWidget::add_tab(Widget *page, const PixBuffer &icon, u8string_view text) {
    bar.add_tab(icon, text);
    display.add_widget(page);
}
void TabWidget::insert_tab(int idx, Widget *page, const PixBuffer &icon, u8string_view text) {
    bar.insert_tab(idx, icon, text);
    display.insert_widget(idx, page);
}
void TabWidget::remove_tab(int idx) {
    bar.remove_tab(idx);
    display.remove_widget(display.widget_of(idx));
}
void TabWidget::delete_tab(int idx) {
    bar.remove_tab(idx);
    display.delete_widget(display.widget_of(idx));
}
void TabWidget::set_current_index(int idx) {
    bar.set_current_index(idx);
    // Because bar's signal_current_changed was connected to StackedWidget, no need to set 
}
void TabWidget::put_internal() {
    if (bar.visible()) {
        // Not hided
        bar.resize(width(), bar.size_hint().h);
        bar.move(x(), y());

        display.move(x(), y() + bar.height());
        display.resize(width(), height() - bar.height());
    }
    else {
        // Hided
        display.move(x(), y());
        display.resize(width(), height());
    }
}
bool TabWidget::resize_event(ResizeEvent &event) {
    put_internal();
    return true;
}
bool TabWidget::move_event(MoveEvent &event) {
    if (bar.visible()) {
        bar.move(event.x(), event.y());
        display.move(event.x(), event.y() + bar.height());
    }
    else {
        display.move(event.x(), event.y());
    }
    return true;
}
bool TabWidget::paint_event(PaintEvent &) {
    if (!paint_background) {
        return true;
    }
    auto &p = painter();
    p.set_brush(palette().input());
    p.fill_rect(display.rect());

    p.set_brush(palette().border());
    // Check if has current
    if (bar.current_index() >= 0 && bar.visible()) {
        auto br = bar.tab_rect(bar.current_index());
        auto dpy = display.rect();

        // Draw Display border
        p.draw_line(dpy.top_left(), dpy.bottom_left());
        p.draw_line(dpy.bottom_left(), dpy.bottom_right());
        p.draw_line(dpy.bottom_right(), dpy.top_right());

        // Draw Line skip the bar
        p.draw_line(dpy.top_left(), FPoint(br.x, dpy.y));
        p.draw_line(FPoint(br.x + br.w, dpy.y), dpy.top_right());
    }
    else  {
        p.draw_rect(display.rect());
    }
    return true;
}
Margin  TabWidget::content_margin() const {
    return display.margin();
}
Widget *TabWidget::current_widget() const {
    return display.current_widget();
}
int     TabWidget::current_index()  const {
    return bar.current_index();
}

BTK_NS_END