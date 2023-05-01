#include "build.hpp"

#include <Btk/widgets/menu.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

PopupWidget::PopupWidget() {
    set_focus_policy(FocusPolicy::Mouse);
}
PopupWidget::~PopupWidget() {

}
void PopupWidget::popup(Widget *wi) {
    set_parent(nullptr);
    if (wi) {
        wi->root()->add_child(this);

        raise();
        take_focus();
    }
    else {
        show();
        raise();
    }
}

bool PopupWidget::paint_event(PaintEvent &) {
    auto &p = painter();
    if (is_window()) {
        // Popup like window
        return true;
    }

    // Draw shadow
    p.save();
    p.translate(2, 2);
    p.set_color(Color::Gray);
    p.fill_rect(FRect(0, 0, size()));
    p.restore();

    p.set_brush(palette().window());
    p.fill_rect(FRect(0, 0, size()));
    p.set_brush(palette().border());
    p.draw_rect(FRect(0, 0, size()));

    return true;
}
bool PopupWidget::focus_lost(FocusEvent &) {
    _focus_lost.emit();
    if (hide_on_focus_lost) {
        hide();
    }
    else {
        close();
    }
    return true;
}
bool PopupWidget::mouse_press(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool PopupWidget::mouse_release(MouseEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool PopupWidget::mouse_motion(MotionEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}
bool PopupWidget::mouse_wheel(WheelEvent &) {
    // Accept it prevent the mouse event processed by lower
    return true;
}

// MenuItem
// Some const, may be mobe into style in future ?
static constexpr auto MenuItemGap = 8 * 2;
static constexpr auto MenuBarHeight = 20;
static constexpr auto PopupMenuItemHeight = 25;
static constexpr auto PopupMenuItemTextOffset = 30;
static constexpr auto PopupMenuItemImageOffset = 5;
static constexpr auto PopupMenuItemSeparatorHeight = 6; 


MenuItem::MenuItem() {

}
MenuItem::~MenuItem() {
    delete _widget;
}
void MenuItem::prepare_at(Widget *parent) {

#if !defined(NDEBUG)
    signal_hovered().connect([this]() {
        BTK_LOG("[MenuItem] hovered %p\n", this);
    });
    signal_triggered().connect([this]() {
        BTK_LOG("[MenuItem] triggered %p\n", this);
    });
#endif

    if (widget()) {
        _widget->set_parent(parent);
        return;
    }
    if (!text().empty()) {
        if (_font.empty()) {
            _font = parent->font();
        }
        _textlay.set_text(text());
        _textlay.set_font(font());
    }
}
void MenuItem::set_image(const PixBuffer &img) {
    _image = img;
    _imgbrush.set_image(img);
}

// Menu
Menu::Menu(Widget *parent, u8string_view text) : Widget(parent) {
    _textlay.set_font(font());
    if (!text.empty()) {
        _textlay.set_text(text);
    }
}
Menu::~Menu() {
    // Release 
    for (auto item : _items) {
        delete item;
    }
}
MenuItem *Menu::add_menu(Menu *menu) {
    if (!menu) {
        return nullptr;
    }
    _items.push_back(new MenuItem(menu));
    return _items.back();
}
MenuItem *Menu::add_item(MenuItem *item) {
    _items.push_back(item);
    item->prepare_at(this);
    return _items.back();
}
MenuItem *Menu::add_separator() {
    auto item = new MenuItem;
    item->_seperator = true;
    return add_item(item);
}
Size Menu::size_hint() const {
    if (_textlay.text().empty()) {
        auto s = style();
        return Size(s->button_width, s->button_height);
    }
    auto [w, h] = _textlay.size();
    return Size(w + MenuItemGap, h);
}
bool Menu::paint_event(PaintEvent &event) {
    if (_textlay.text().empty()) {
        return true;
    }
    auto &p = painter();
    auto [x, y] = FRect(0, 0, size()).cast<float>().center();

    if (under_mouse()) {
        p.set_color(Color::LightGray);
        p.fill_rect(FRect(0, 0, size()));
    }

    p.set_brush(palette().text());
    p.set_text_align(AlignMiddle | AlignCenter);
    p.draw_text(_textlay, x, y);
    return true;
}
bool Menu::mouse_press(MouseEvent &event) {
    // Check items
    if (!_items.empty()) {
        auto popup = new PopupMenu;
        popup->borrow_items_from(_items);
        popup->set_attribute(WidgetAttrs::DeleteOnClose, true);
        popup->popup();

        auto [sx, sy] = map_from_self_to_screen(Rect(0, 0, size()).bottom_left());
        // popup->set_window_position(sx, sy);
        popup->move(sx, sy);
    }

    return false; //< For pass throught to the lower widget, call triggered
}
bool Menu::mouse_release(MouseEvent &event) {
    return false; //< For pass throught to the lower widget, call triggered
}
bool Menu::mouse_motion(MotionEvent &event) {
    return false; //< For pass throught to the lower widget, call hovered
}

// MenuBar
MenuBar::MenuBar(Widget *parent) : Widget(parent) {
    resize(style()->button_width, MenuBarHeight);
}
MenuBar::~MenuBar() {
    // Release 
    for (auto item : _items) {
        delete item;
    }
}
MenuItem *MenuBar::add_menu(Menu *submenu) {
    auto item = add_item(new MenuItem(submenu));
    return item;
}
MenuItem *MenuBar::add_item(MenuItem *item) {
    _items.push_back(item);

    // Pending a request
    repaint();

    item->prepare_at(this);
    if (item->widget()) {
        _num_widget_item += 1;
        relayout();
    }

    // Ask parent to relayout
    Event event(Event::LayoutRequest);
    event.set_timestamp(GetTicks());
    if (parent()) {
        parent()->handle(event);
    }

    return _items.back();
}
MenuItem *MenuBar::add_separator() {
    auto item = new MenuItem;
    item->_seperator = true;
    return add_item(item);
}
void MenuBar::relayout() {
    if (_num_widget_item == 0) {
        return;
    }
    auto lasted = _num_widget_item;
    for (auto item : _items) {
        auto wi = item->widget();
        if (wi) {
            lasted -= 1;
            wi->set_parent(this);
            // To set the right place
            auto rect = item_rect(item);
            wi->set_rect(rect);
        }
        if (lasted == 0) {
            // No more
            break;
        }
    }
}
bool MenuBar::mouse_press(MouseEvent &event) {
    auto [x, y] = event.position();
    auto item = item_at(x, y);
    if (item) {
        item->signal_triggered().emit();
    }
    return true;
}
bool MenuBar::mouse_release(MouseEvent &event) {
    return true;
}
bool MenuBar::mouse_motion(MotionEvent &event) {
    _last_motion = GetTicks();
    _hovered_item = nullptr;
    auto [x, y] = event.position();
    auto new_item = item_at(x, y);
    if (new_item != _current_item) {
        _current_item = new_item;
        repaint();
    }
    return true;
}
bool MenuBar::mouse_enter(MotionEvent &) {
    _hovered_timer = add_timer(TimerType::Coarse, _hovered_time);
    return true;
}
bool MenuBar::mouse_leave(MotionEvent &) {
    _current_item = nullptr;
    repaint();

    del_timer(_hovered_timer);
    _hovered_timer = 0;
    return true;
}
bool MenuBar::move_event(MoveEvent &) {
    relayout();
    return true;
}
bool MenuBar::resize_event(ResizeEvent &) {
    relayout();
    return true;
}
bool MenuBar::timer_event(TimerEvent &event) {
    if (event.timerid() != _hovered_timer) {
        return false;
    }
    // BTK_LOG("[PopupMenu] timestamps %d\n", int(event.timestamp()));
    if (event.timestamp() - _last_motion > _hovered_time && _current_item && _hovered_item != _current_item) {
        _hovered_item = _current_item;
        BTK_LOG("[MenuBar] hovered detected for this %p : %s\n", _hovered_item, _hovered_item->text().data());
        _hovered_item->signal_hovered().emit();
    }
    return true;
}
bool MenuBar::paint_event(PaintEvent &) {
    auto &p = painter();

    p.set_text_align(AlignMiddle | AlignCenter);

    // Draw basic
    p.set_brush(palette().input());
    p.fill_rect(FRect(0, 0, size()));
    // p.set_brush(palette().border());
    // p.draw_line(rect().bottom_left(), rect().bottom_right());

    // Begin Draw items
    p.set_brush(palette().text());
    for (auto item : _items) {
        auto bar = item_rect(item);

        // Draw base if is current
        if (item->widget()) {
            // Is widget, just skip
            continue;
        }
        if (item == _current_item && !item->is_seperator()) {
            p.set_color(Color::LightGray);
            p.fill_rounded_rect(bar, 0);
            p.set_brush(palette().text());
        }

        // Draw text if
        if (item->is_seperator()) {
            p.set_color(Color::LightGray);
            p.draw_line(bar.x + bar.w / 2, bar.y, bar.x + bar.w / 2, bar.y + bar.h);
            p.set_brush(palette().text());
        }
        if (!item->text().empty()) {
            auto [x, y] = bar.cast<float>().center();
            p.draw_text(item->_textlay, x, y);
        }
    }
    return true;
}
Rect MenuBar::item_rect(MenuItem *item) const {
    Rect host = FRect(0, 0, size());
    Rect r = host;
    r.w = 0;

    for (auto cur : _items) {
        if (!cur->text().empty()) {
            r.w = cur->_textlay.size().w + MenuItemGap;
        }
        if (cur->is_seperator()) {
            r.w = 8;
        }
        if (cur->widget()) {
            r.w = cur->widget()->size_hint().w;
        }
        if (cur == item) {
            break;
        }
        r.x += r.w;
    }
    return r;
}
MenuItem *MenuBar::item_at(int x, int y) const {
    for (auto cur : _items) {
        if (item_rect(cur).contains(x, y)) {
            return cur;
        }
    }
    return nullptr;
}
Size      MenuBar::size_hint() const {
    if (_items.empty()) {
        return Size(0, MenuBarHeight);
    }
    auto first = item_rect(_items.front());
    auto last = item_rect(_items.back());

    return Size(last.top_right().x - first.x, max(first.h, last.h));
}
int        MenuBar::count_items() const {
    return _items.size();
}

// PopupMenu
PopupMenu::PopupMenu(Widget *parent) : Widget(parent, WindowFlags::Borderless | WindowFlags::PopupMenu) {
    resize(200, 200);
}
PopupMenu::~PopupMenu() {
    if (_items_owned) {
        for (auto item : _items) {
            delete item;
        }
    }
}
void PopupMenu::popup() {
    auto last = item_rect(_items.back());
    show();
    resize(last.w + 2, last.y + last.h + 4);
}
bool PopupMenu::mouse_motion(MotionEvent &event) {
    _last_motion = GetTicks();
    _hovered_item = nullptr;
    auto n = item_at(event.position());
    if (n != _current_item) {
        _current_item = n;
        repaint();
    }
    return true;
}
bool PopupMenu::mouse_press(MouseEvent &event) {
    auto n = item_at(event.position());
    if (n && !n->is_seperator()) {
        _ready_close = true;
        n->signal_triggered().emit();
        close();
    }
    return true;
}
bool PopupMenu::mouse_release(MouseEvent &) {
    return true;
}
bool PopupMenu::mouse_enter(MotionEvent &event) {
    // BTK_LOG("[PopupMenu] mouse enter, begin detect hovered\n");
    _hovered_timer = add_timer(TimerType::Coarse, _hovered_time);
    return true;
}
bool PopupMenu::mouse_leave(MotionEvent &event) {
    // BTK_LOG("[PopupMenu] mouse leave, end detect hovered\n");
    del_timer(_hovered_timer);
    _hovered_timer = 0;
    return true;
}
bool PopupMenu::timer_event(TimerEvent &event) {
    if (event.timerid() != _hovered_timer) {
        return false;
    }
    // BTK_LOG("[PopupMenu] timestamps %d\n", int(event.timestamp()));
    if (event.timestamp() - _last_motion > _hovered_time && _current_item && _hovered_item != _current_item) {
        _hovered_item = _current_item;
        BTK_LOG("[PopupMenu] hovered detected for this %p : %s\n", _hovered_item, _hovered_item->text().data());
        _hovered_item->signal_hovered().emit();
    }
    return true;
}
bool PopupMenu::focus_lost(FocusEvent &event) {
    if (!_ready_close) {
        close();
    }
    return true;
}
bool PopupMenu::close_event(CloseEvent &) {
    if (_current_poping) {
        // Sub popup
        _current_poping->close();
        _current_poping = nullptr;
    }
    return false;
}
bool PopupMenu::paint_event(PaintEvent &) {
    auto &p = painter();
    // Fill background
    p.set_brush(palette().input());
    p.fill_rect(FRect(0, 0, size()));

    // Draw Items 
    p.set_text_align(AlignLeft | AlignTop);
    p.set_brush(palette().text());
    for (auto item : _items) {
        auto rect = item_rect(item).cast<float>();

        if (item == _current_item && !item->is_seperator()) {
            p.set_color(Color::LightGray);
            p.fill_rect(rect);
            p.set_brush(palette().text());
        }
        if (item->is_seperator()) {
            p.set_color(Color::LightGray);
            p.draw_line(rect.x, rect.y + rect.h / 2, rect.x + rect.w, rect.y + rect.h / 2);
            p.set_brush(palette().text());
            continue;
        }
        if (!item->_text.empty()) {
            auto aligned = rect.align_object(item->_textlay.size(), AlignLeft | AlignMiddle);
            p.draw_text(item->_textlay, aligned.x + PopupMenuItemTextOffset, aligned.y);

        }
        if (!item->_image.empty()) {
            auto s = style();
            auto icon_rect = rect.align_object(FSize(s->icon_width, s->icon_height), AlignLeft | AlignMiddle);
            icon_rect.x += PopupMenuItemImageOffset;
            p.set_brush(item->_imgbrush);
            p.fill_rect(icon_rect);
            p.set_brush(palette().text());
        }

    }
    return true;
}
Rect PopupMenu::item_rect(MenuItem *item) {
    Rect r;
    r.x = 2;
    r.y = 2;
    r.w = width();
    r.h = 0;
    for (auto cur : _items) {
        if (!cur->text().empty()) {
            r.h = max<int>(cur->_textlay.size().h, PopupMenuItemHeight); // At least has a button height
        }
        else if (cur->is_seperator()) {
            r.h = PopupMenuItemSeparatorHeight;
        }
        else {
            r.h = PopupMenuItemHeight;
        }
        if (cur == item) {
            break;
        }
        r.y += r.h;
    }
    return r;
}
MenuItem *PopupMenu::item_at(const Point &where) {
    for (auto cur : _items) {
        auto r = item_rect(cur);
        if (r.contains(where)) {
            return cur;
        }
    }
    return nullptr;
}
MenuItem *PopupMenu::add_menu(Menu *submenu) {
    auto item = add_item(new MenuItem(submenu));
    return item;
}
MenuItem *PopupMenu::add_item(MenuItem *item) {
    _items.push_back(item);

    item->prepare_at(this);

    return _items.back();
}
MenuItem *PopupMenu::add_separator() {
    auto item = new MenuItem;
    item->_seperator = true;
    return add_item(item);
}
void PopupMenu::borrow_items_from(const std::vector<MenuItem*> &borrowed) {
    _items = borrowed;
    _items_owned = false;
}

BTK_NS_END