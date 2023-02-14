#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/painter.hpp>
#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

Widget::Widget(Widget *parent) {
    _context = GetUIContext();
    BTK_ASSERT(_context);

    if (parent != nullptr) {
        // Inherit from parent
        _style   = parent->_style;
        _palette = parent->_palette;
        _font    = parent->_font;

        parent->add_child(this);
    }
    else {
        _style   = &_context->style;
        _palette = _context->palette;
        _font    = _style->font;
    }
}
Widget::~Widget() {

    // Auto detach from parent
    if(_in_child_iter != std::list<Widget *>::iterator{}){
        parent()->_children.erase(_in_child_iter);

        // Notify parent
        Event event(Event::ChildRemoved);
        _parent->handle(event);
    }
    // Clear children
    for(auto w : _children) {
        // Set iter to {}
        w->_in_child_iter = std::list<Widget *>::iterator{};
        delete w;
    }
    // Destroy window if needed
    if (is_window()) {
        window_destroy();
    }
}

void Widget::set_visible(bool v) {
    if (is_root()) {
        //Top level widget
        if (_win == nullptr) {
            window_init();
        }
        _win->show(v);
    }
    _visible = v;
    Event event(v ? Event::Show : Event::Hide);
    handle(event);

    if (_parent) {
        event.set_type(Event::LayoutRequest);
        _parent->handle(event);
    }
}
void Widget::show() {
    set_visible(true);
}
void Widget::hide() {
    set_visible(false);
}
void Widget::raise() {
    if (_win) {
        _win->raise();
    }
    if (parent()) {
        parent()->_children.erase(_in_child_iter);
        parent()->_children.push_front(this);

        _in_child_iter = parent()->_children.begin();
        repaint();
    }
}
void Widget::lower() {
    if (parent()) {
        parent()->_children.erase(_in_child_iter);
        parent()->_children.push_back(this);

        _in_child_iter = --parent()->_children.end();
        repaint();
    }
}
void Widget::close() {
    if (is_window()) {
        _win->close();
        return;
    }
    // Try generate close event
    WidgetEvent event(Event::Close);
    event.set_timestamp(GetTicks());
    event.set_widget(this);
    event.accept();

    handle(event);

    if (event.is_accepted()) {
        set_visible(false);
    }
}
void Widget::resize(int w, int h) {
    // Clamp value by minimum & maximum
    w = clamp(w, _minimum_size.w, _maximum_size.w);
    h = clamp(h, _minimum_size.h, _maximum_size.h);

    int old_w = _rect.w;
    int old_h = _rect.h;
    _rect.w = w;
    _rect.h = h;
    if (_win != nullptr) {
        _win->resize(w, h);
    }
    ResizeEvent event;
    event.set_new_size(w, h);
    event.set_old_size(old_w, old_h);
    handle(event);

    // Check mouse layout etc...
    rectangle_update();

    // We should repaint 
    repaint();
}
void Widget::move(int x, int y) {
    int old_x = _rect.x;
    int old_y = _rect.y;
    _rect.x = x;
    _rect.y = y;

    MoveEvent event(x, y);
    handle(event);

    // Check mouse layout etc...
    rectangle_update();

    repaint();
}
void Widget::set_rect(int x, int y, int w, int h) {
    resize(w, h);
    move(x, y);
}
void Widget::set_palette(const Palette &palette) {
    _palette = palette;

    WidgetEvent event(Event::PaletteChanged);
    event.set_timestamp(GetTicks());
    event.set_widget(this);
    handle(event);

    repaint();
}
bool Widget::handle(Event &event) {
    // printf("Widget::handle(%d)\n", event.type());
    // Do event filters
    if (run_event_filter(event)) {
        // Filtered
        return true;
    }

    // Default unhandled
    bool ret = false;

    switch (event.type()) {
        case Event::Paint : {
            if (_win) {
                auto style = _style;
                auto c     = style->window;
                _painter.set_color(c.r, c.g, c.b, c.a);
                _painter.begin();
                _painter.clear();
            }

            // Paint current widget first (background)
            ret = paint_event(event.as<PaintEvent>());

            // Paint children second (foreground)
            paint_children(event.as<PaintEvent>());

            if (_win) {

#if            !defined(NDEBUG)
                if ((_attrs & WidgetAttrs::Debug) == WidgetAttrs::Debug) {
                    debug_draw();
                }
#endif
                _painter.end();
            }
            break;
        }
        case Event::Hide : {
            _visible = false;
            break;
        }
        case Event::Show : {
            _visible = true;
            break;
        }
        case Event::Resized : {
            auto &e = static_cast<ResizeEvent &>(event);
            if (is_window()) {
                // Size w, h
                _rect.x = 0;
                _rect.y = 0;
                _rect.w = e.width();
                _rect.h = e.height();

                // Painter notify
                if (_win != nullptr) {
                    _painter.notify_resize(e.width(), e.height());
                }
            }
            return resize_event(e);
        }
        case Event::Moved : {
            return move_event(event.as<MoveEvent>());
        }
        case Event::KeyPress : {
            if (focused_widget) {
                if (focused_widget->handle(event)) {
                    return true;
                }
            }
            else if (mouse_current_widget) {
                // No focused widget, try current widget
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            
            // No focused widget, or unhandled by focused widget
            return key_press(event.as<KeyEvent>());
        }
        case Event::KeyRelease : {
            if (focused_widget) {
                if (focused_widget->handle(event)) {
                    return true;
                }
            }
            else if (mouse_current_widget) {
                // No focused widget, try current widget
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            return key_release(event.as<KeyEvent>());
        }
        case Event::MousePress : {
            _pressed = true;
            if (focused_widget) {
                auto &mouse = event.as<MouseEvent>();
                // Try reset previous focused widget
                if (!focused_widget->_rect.contains(mouse.position())) {
                    BTK_LOG("Focus out %s\n", Btk_typename(focused_widget));
                    Event event(Event::FocusLost);
                    focused_widget->handle(event);
                    focused_widget = nullptr;
                }
            }

            if (mouse_current_widget) {
                // Try Make a new focused widget
                if (uint8_t(mouse_current_widget->_focus & FocusPolicy::Mouse)) {
                    if (focused_widget == nullptr) {
                        // Has this bit, Ok to focus
                        BTK_LOG("Focus on %s\n", Btk_typename(mouse_current_widget));
                        focused_widget = mouse_current_widget;
                        Event event(Event::FocusGained);
                        focused_widget->handle(event);
                    }
                }
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            // Unhandled or no widget under mouse
            return mouse_press(event.as<MouseEvent>());
        }
        case Event::MouseRelease : {
            // Reset pressed flag and drag flag
            _drag_reject = false;
            _pressed = false;
            if (dragging_widget && is_root()) {
                BTK_LOG("Widget::handle(MouseRelease) - Top level, Generate DragEnd\n");
                BTK_LOG("Widget::handle(MouseRelease) - Dragging widget %p\n", dragging_widget);
                // Only root widget can generate drag event
                // Send this event to drag widget
                auto mouse = event.as<MouseEvent>();
                DragEvent event(Event::DragEnd, mouse.x(), mouse.y());
                dragging_widget->handle(event);
                dragging_widget = nullptr;
                // Uncapture mouse
                capture_mouse(false);
            }
            if (mouse_current_widget) {
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            // Unhandled or no widget under mouse
            return mouse_release(event.as<MouseEvent>());
        }
        case Event::MouseEnter : {
            _entered = true;
            if (mouse_current_widget) {
                mouse_current_widget->handle(event);
            }
            return mouse_enter(event.as<MotionEvent>());
        }
        case Event::MouseLeave : {
            // Reset current widget
            _entered = false;
            if (mouse_current_widget) {
                mouse_current_widget->handle(event);
                mouse_current_widget = nullptr;
            }
            return mouse_leave(event.as<MotionEvent>());
        }
        case Event::MouseMotion : {
            // TODO : We should improve it by restore / save
            // Set cursor if
            if (!_cursor.empty()) {
                _cursor.set();
            }

            // Get current mouse widget
            auto &motion = event.as<MotionEvent>();

            // TODO : generate Drag here
            if (_pressed && is_root() && !_drag_reject) {
                if (!_drag_reject && !dragging_widget) {
                    // Try start it
                    DragEvent event(Event::DragBegin, motion.x(), motion.y());
                    event.set_rel(motion.xrel(), motion.yrel());

                    bool v = handle(event);
                    // Check widget has founded or self accept it
                    if (!dragging_widget && !v) {
                        BTK_LOG("Widget::handle(MouseMotion) - No widget under mouse\n");
                        BTK_LOG("Widget::handle(MouseMotion) - Drag rejected\n");
                        // No widget under mouse
                        // Reject drag
                        _drag_reject = true;
                    }
                    else {
                        BTK_LOG("Widget::handle(MouseMotion) - Drag begin\n");
                        BTK_LOG("Widget::handle(MouseMotion) - Dragging widget '%s'\n", Btk_typename(dragging_widget));
                        BTK_LOG("Widget::handle(MouseMotion) - Capture mouse\n");
                        capture_mouse(true);
                    }
                }
                DragEvent event(DragEvent::DragMotion, motion.x(), motion.y());
                event.set_rel(motion.xrel(), motion.yrel());
                if (dragging_widget) {
                    dragging_widget->handle(event);
                }
                else if(!_drag_reject) {
                    // Top level self accept drag
                    handle(event);
                }
            }

            if (mouse_current_widget == nullptr) {
                // Try get one
                auto w = child_at(motion.x(), motion.y());
                if (w != nullptr) {
                    // Notify mouse enter
                    WidgetEvent event(Event::MouseEnter);
                    event.set_timestamp(motion.timestamp());
                    event.set_widget(w);

                    w->handle(event);
                }
                mouse_current_widget = w;
            }
            else {
                // Notify mouse motion
                bool ret = false;
                if (mouse_current_widget->handle(motion)) {
                    // It already handled, no need to notify parent
                    ret = true;
                }

                // Check if leave
                if (!mouse_current_widget->rect().contains(motion.position())) {
                    // Leave
                    WidgetEvent event(Event::MouseLeave);
                    event.set_timestamp(motion.timestamp());
                    event.set_widget(mouse_current_widget);

                    mouse_current_widget->handle(event);
                    mouse_current_widget = nullptr;
                }

                if (ret) {
                    return ret;
                }
            }

            // No current mouse widget or unhandled motion event, notify self
            return mouse_motion(motion);
        }
        case Event::MouseWheel : {
            if (mouse_current_widget) {
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            return mouse_wheel(event.as<WheelEvent>());
        }
        case Event::Close : {
            ret = close_event(event.as<CloseEvent>());
            if (!ret && bittest(_attrs, WidgetAttrs::DeleteOnClose)) {
                // Ready to close & has attribute delete on close
                defer_delete();
            }
            return ret;
        }
        case Event::Timer : {
            return timer_event(event.as<TimerEvent>());
        }
        // TODO : Support drag and drop
        case Event::DragBegin : {
            auto drag = event.as<DragEvent>();
            auto child = child_at(drag.x(), drag.y());
            if (child) {
                // Got a child, notify it
                dragging_widget = child;
                if (child->handle(event)) {
                    return true;
                }
            }
            // Unhandled or no child under mouse
            if (drag_begin(drag)) {
                // Self accept it
                dragging_widget = this;
            }
            return false;
        }
        case Event::DragEnd : {
            auto drag = event.as<DragEvent>();
            if (dragging_widget && dragging_widget != this) {
                // Notify drag end
                dragging_widget->handle(event);
            }
            dragging_widget = nullptr;
            return drag_end(drag);
        }
        case Event::DragMotion : {
            // It may is self accepted this drag
            if (dragging_widget && dragging_widget != this) {
                if (dragging_widget->handle(event)) {
                    return true;
                }
            }
            // Unhandled or no widget under mouse
            return drag_motion(event.as<DragEvent>());
        }
        case Event::TextInput : {
            if (focused_widget) {
                if (focused_widget->handle(event)) {
                    return true;
                }
            }
            if (mouse_current_widget) {
                if (mouse_current_widget->handle(event)) {
                    return true;
                }
            }
            return textinput_event(event.as<TextInputEvent>());
        }
        case Event::FocusGained : {
            _focused = true;
            return focus_gained(event.as<FocusEvent>());
        }
        case Event::FocusLost : {
            if (focused_widget) {
                if (focused_widget->handle(event)) {
                    BTK_LOG("Focus out %s\n", Btk_typename(focused_widget));
                    ret = true;
                }
                focused_widget = nullptr;
            }
            _focused = false;
            return focus_lost(event.as<FocusEvent>());
        }
        case Event::ChildRectangleChanged :
        case Event::PaletteChanged :
        case Event::StyleChanged :
        case Event::FontChanged : {
            repaint();
            return change_event(event.as<ChangeEvent>());
        }
        default: {
            break;
        }
    }

    return ret;
}
void Widget::repaint() {
    // TODO Check is GUI Thread
    // if (!_visible) {
    //     return;
    // }
    if (!is_window()) {
        return root()->repaint();
    }
    if (_win) {
        _win->repaint();
    }
}
void Widget::repaint_now() {
    if (!is_window()) {
        return root()->repaint_now();
    }
    if (_win) {
        PaintEvent event;
        event.set_widget(this);
        event.set_timestamp(GetTicks());
        handle(event);
    }
}

// Query

Rect Widget::rect() const {
    return _rect;
}
bool Widget::visible() const {
    return _visible;
}
bool Widget::is_window() const {
    // Is top level widget or _win is not nullptr
    return _win != nullptr || _parent == nullptr;
}
bool Widget::is_root() const {
    return _parent == nullptr;
}
bool Widget::has_focus() const {
    return _focused;
}
Widget *Widget::parent() const {
    return _parent;
}
Style  *Widget::style() const {
    return _style;
}
auto   Widget::font() const -> const Font & {
    return _font;
}
auto   Widget::palette() const -> const Palette & {
    return _palette;
}

// Palette
auto    Widget::set_palette_current_group(Palette::Group gp) -> void {
    _palette.set_current_group(gp);
}

// Size Hint
Size Widget::adjust_size() const {
    auto ret = size_hint();
    if (ret.is_valid()) {
        return ret;
    }
    // Unit all children
    Rect r = {0, 0, 0, 0};
    for (auto &child : _children) {
        r = r.united(child->rect());
    }
    return r.size();
}
Size Widget::size_hint() const {
    return Size(0, 0);
}

window_t Widget::winhandle() const {
    if (_parent == nullptr) {
        return _win;
    }
    return nullptr;
}
driver_t Widget::driver() const {
    return _context->driver();
}
Painter &Widget::painter() const {
    // if (priv->gc == nullptr) {
    //     // To top level widget
    //     auto prev = priv->parent;
    //     while (prev->priv->parent != nullptr) {
    //         prev = prev->priv->parent;
    //     }
    //     priv->gc = prev->priv->gc;
    // }
    // return priv->gc;
    return root()->_painter;
}

// Child widgets
Widget *Widget::child_at(int x, int y) const {
    for(auto w : _children) {
        if (w->_visible && w->rect().contains(x, y)) {
            return w;
        }
    }
    return nullptr;
}
void Widget::add_child(Widget *w) {
    if (w == nullptr) {
        return;
    }
    assert(w->parent() == nullptr);

    // Set parent
    w->_parent = this;
    // Add to children
    _children.push_front(w);
    // Set iterator
    w->_in_child_iter = _children.begin();

    // Notify
    WidgetEvent event(Event::ChildAdded);
    event.set_widget(w);
    handle(event);
}
void Widget::remove_child(Widget *w) {
    if (w == nullptr) {
        return;
    }
    // Find in children
    auto it = std::find(_children.begin(), _children.end(), w);
    if (it == _children.end()) {
        return;
    }

    // Remove from children
    w->_in_child_iter = {};
    _children.erase(it);

    // Notify
    WidgetEvent event(Event::ChildRemoved);
    event.set_widget(w);
    handle(event);
}
bool Widget::has_child(Widget *w) const {
    auto it = std::find(_children.begin(), _children.end(), w);
    return it != _children.end();
}
void Widget::set_parent(Widget *w) {
    if (parent() == w) {
        return ;
    }
    if (parent()) {
        parent()->remove_child(this);
    }
    if (w) {
        w->add_child(this);
    }
}
void Widget::paint_children(PaintEvent &event) {
    // From bottom to top
    for(auto iter = _children.rbegin(); iter != _children.rend(); ++iter) {
        auto w = *iter;
        if (!w->_visible || w->_rect.empty()) {
            continue;
        }
        w->handle(event);
    }
}

// Root
Widget *Widget::root() const {
    if (_parent == nullptr) {
        return const_cast<Widget*>(this);
    }
    Widget *cur = _parent;

    while(cur->_parent != nullptr) {
        cur = cur->_parent;
    }
    return cur;
}
Widget *Widget::window() const {
    auto p = root();
    if (p == nullptr || !p->is_window()) {
        return nullptr;
    }
    return p;
}

// Query window
FPoint Widget::window_dpi() const {
    FPoint dpi = {96.0f, 96.0f};
    auto w = window();
    if (w && w->_win) {
        w->_win->query_value(AbstractWindow::Dpi, &dpi);
    }
    else {
        driver()->query_value(GraphicsDriver::SystemDpi, &dpi);
    }
    return dpi;
}

// Window configure
void Widget::set_window_title(u8string_view title) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->set_title(title);
    }
}
void Widget::set_window_size(int w, int h) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->resize(w, h);
    }
}
void Widget::set_window_position(int x, int y) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->move(x, y);
    }
}
void Widget::set_window_icon(const PixBuffer &icon) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->set_icon(icon);
    }
}
bool Widget::set_window_flags(WindowFlags f) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _flags = f;
        return _win->set_flags(_flags);
    }
    return false;
}
bool Widget::set_window_borderless(bool v) {
    // if (is_window()) {
    //     if (!_win) {
    //         window_init();
    //     }
    //     _win->set_flags(win, varg_bool_t(v));
    // }
    return false;
}
bool Widget::set_fullscreen(bool v) {
    if (v) {
        _flags |= WindowFlags::Fullscreen;
    }
    else {
        _flags ^= WindowFlags::Fullscreen;
    }
    return set_window_flags(_flags);
}
bool Widget::set_resizable(bool v) {
    if (v) {
        _flags |= WindowFlags::Resizable;
    }
    else {
        _flags ^= WindowFlags::Resizable;
    }
    return set_window_flags(_flags);
}
void Widget::set_attribute(WidgetAttrs attr, bool on) {
    if (on) {
        _attrs |= attr;
    }
    else {
        _attrs ^= attr;
    }
}

// Mouse
void Widget::capture_mouse(bool capture) {
    if (is_window()) {
        // Need we need more strict check ?
        if (!_win) {
            window_init();
        }
        _win->capture_mouse(capture);
    }
}
bool Widget::warp_mouse(int x, int y) {
    auto win = root()->_win;
    if (win) {
        Point p(x, y);
        return win->set_value(AbstractWindow::MousePosition, &p);
    }
    return false;
}
Point Widget::mouse_position() const {
    auto win = root()->_win;
    Point p(0, 0);
    if (win) {
        win->query_value(AbstractWindow::MousePosition, &p);
    }
    return p;
}

// IME
void Widget::start_textinput(bool v) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->start_textinput(v);
    }
    // Try get root widget and start textinput
    if (!is_root()) {
        return root()->start_textinput(v);
    }
    // ???
}
void Widget::set_textinput_rect(const Rect &r) {
    if (is_window()) {
        if (!_win) {
            window_init();
        }
        _win->set_textinput_rect(r);
    }
    if (!is_root()) {
        return root()->set_textinput_rect(r);
    }
}

// Properties
void Widget::set_focus_policy(FocusPolicy policy) {
    _focus = policy;

    // Check parent focus widget, and remove focus widget if need
    if (_parent) {
        if (_parent->focused_widget == this) {
            Event event(Event::FocusLost);
            handle(event);
            _parent->focused_widget = nullptr;
        }
    }
}
void Widget::set_size_policy(SizePolicy policy) {
    _size = policy;
}
void Widget::set_maximum_size(int w, int h) {
    Size s(w, h);

    _maximum_size = s;
    if (_win) {
        _win->set_value(AbstractWindow::MaximumSize, &s);
    }
}
void Widget::set_minimum_size(int w, int h) {
    Size s(w, h);

    _minimum_size = s;
    if (_win) {
        _win->set_value(AbstractWindow::MinimumSize, &s);
    }
}
void Widget::set_cursor(const Cursor &cursor) {
    _cursor = cursor;
}
void Widget::set_font(const Font &font) {
    _font = font;
    Event event(Event::FontChanged);
    handle(event);
}
void Widget::set_style(Style *s) {
    _style = s;
    Event event(Event::StyleChanged);
    handle(event);
}

// Window Internal
void Widget::window_init() {
    if (_win) {
        return;
    }
    Size size = _rect.size();
    // Try to use adjust size
    if (size.w <= 0 || size.h <= 0) {
        size = adjust_size();
    }
    // Still no size, use default
    if (size.w <= 0 || size.h <= 0) {
        size = {_style->window_width, _style->window_height};
    }
    _rect.w = size.w;
    _rect.h = size.h;

    _win = driver()->window_create(
        u8string_view(),
        size.w,
        size.h,
        _flags
    );
    _win->bind_widget(this);
    _painter = Painter::FromWindow(_win);
    _painter_inited = true;
}
void Widget::window_destroy() {
    if (_win) {
        // Release painter first
        _painter = {};
        _painter_inited = false;
        delete _win;
        _win = nullptr;
    }
}
void Widget::rectangle_update() {
    // Check parent if we are mouse is out
    bool has_mouse = false;
    Point mouse;
    if (_parent) {
        // We need a way to get the mouse position
        auto w = window();
        if (w && w->_win) {
            if (w->_win->query_value(AbstractWindow::MousePosition, &mouse)) {
                has_mouse = true;
            }
        }
    }

    auto check_mouse = [&, this]() {
        MotionEvent event(mouse.x, mouse.y);
        auto &mouse_current = _parent->mouse_current_widget;

        if (mouse_current) {
            if (mouse_current->_rect.contains(mouse)) {
                // Unchanged
                return;
            }
            BTK_LOG(
                "Widget::rectangle_update() mouse leave widget %p '%s'\n", 
                mouse_current, 
                Btk_typename(mouse_current)
            );
            event.set_type(Event::MouseLeave);
            mouse_current->handle(event);
            mouse_current = nullptr;
        }
        auto w = _parent->child_at(mouse.x, mouse.y);
        if (w) {
            BTK_LOG(
                "Widget::rectangle_update() mouse enter widget %p '%s'\n", 
                w, 
                Btk_typename(w)
            );
            mouse_current = w;
            event.set_type(Event::MouseEnter);
            w->handle(event);
        }
    };

    if (has_mouse) {
        check_mouse();
    }

    if (_parent) {
        Event event(Event::ChildRectangleChanged);
        _parent->handle(event);
    }
}
void Widget::debug_draw() {
    for (auto child : _children) {
        child->debug_draw();
    }
    auto &p = painter();
    p.set_color(Color::Red);
    p.draw_rect(_rect);
}

// Cursor
Cursor::Cursor() {
    cursor = nullptr;
}
Cursor::Cursor(SystemCursor sys) {
    auto driver = GetDriver();
    BTK_ASSERT(driver);
    cursor = driver->cursor_create(sys);
}
Cursor::Cursor(const PixBuffer &pixbuf, int hot_x, int hot_y) {
    auto driver = GetDriver();
    BTK_ASSERT(driver);
    cursor = driver->cursor_create(pixbuf, hot_x, hot_y);
}
Cursor::Cursor(const Cursor &c) {
    cursor = COW_REFERENCE(c.cursor);
}
Cursor::~Cursor() {
    COW_RELEASE(cursor);
}
bool Cursor::empty() const {
    return !cursor;
}
void Cursor::set() const {
    if (cursor) {
        cursor->set();
    }
}
Cursor &Cursor::operator =(const Cursor &c) {
    if (this == &c) {
        return *this;
    }
    COW_RELEASE(cursor);
    cursor = COW_REFERENCE(c.cursor);
    return *this;
}

BTK_NS_END