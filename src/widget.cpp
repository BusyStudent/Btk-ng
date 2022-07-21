#include "build.hpp"

#include <Btk/painter.hpp>
#include <Btk/context.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

Widget::Widget() {
    _context = GetUIContext();
    _style   = &_context->style;
    _font    = _style->font;

    BTK_ASSERT(_context);
}
Widget::Widget(Widget *parent) : Widget() {
    if (parent != nullptr) {
        parent->add_child(this);
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


    // Release painter
    _painter = {};
}

void Widget::set_visible(bool v) {
    if (parent() == nullptr) {
        //Top level widget
        if (_win == nullptr) {
            Size size = _rect.size();
            // Try to use size hint
            if (size.w == 0 || size.h == 0) {
                size = size_hint();
            }
            // Still no size, use default
            if (size.w == 0 || size.h == 0) {
                size = {100, 100};
                _rect.w = size.w;
                _rect.h = size.h;
            }

            _win = driver()->window_create(
                nullptr,
                size.w,
                size.h,
                _flags
            );
            _win->bind_widget(this);
            _painter = _win->painter_create();
            _painter_inited = true;
        }
        _win->show(v);
    }
    _visible = v;
    Event event(v ? Event::Show : Event::Hide);
    handle(event);
}
void Widget::show() {
    set_visible(true);
}
void Widget::hide() {
    set_visible(false);
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
}
void Widget::move(int x, int y) {
    int old_x = _rect.x;
    int old_y = _rect.y;
    _rect.x = x;
    _rect.y = y;

    MoveEvent event(x, y);
    handle(event);
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
            // From bottom to top
            for(auto iter = _children.rbegin(); iter != _children.rend(); ++iter) {
                auto w = *iter;
                if (!w->_visible || w->_rect.empty()) {
                    continue;
                }
                w->handle(event);
            }

            if (_win) {
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
                _painter.notify_resize(e.width(), e.height());
            }
            break;
        }
        case Event::KeyPress : {
            if (focused_widget) {
                if (focused_widget->handle(event)) {
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
            return key_release(event.as<KeyEvent>());
        }
        case Event::MousePress : {
            _pressed = true;
            if (mouse_current_widget) {
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
            if (mouse_current_widget) {
                mouse_current_widget->handle(event);
            }
            return mouse_enter(event.as<MotionEvent>());
        }
        case Event::MouseLeave : {
            // Reset current widget
            if (mouse_current_widget) {
                mouse_current_widget->handle(event);
                mouse_current_widget = nullptr;
            }
            return mouse_leave(event.as<MotionEvent>());
        }
        case Event::MouseMotion : {
            // Get current mouse widget
            auto &motion = event.as<MotionEvent>();

            // TODO : generate Drag here
            if (_pressed && is_root() && !_drag_reject) {
                if (!_drag_reject && !dragging_widget) {
                    // Try start it
                    DragEvent event(Event::DragBegin, motion.x(), motion.y());
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
            return drag_begin(drag);
        }
        case Event::DragEnd : {
            auto drag = event.as<DragEvent>();
            if (dragging_widget) {
                // Notify drag end
                dragging_widget->handle(event);
                dragging_widget = nullptr;
            }
            return drag_end(drag);
        }
        case Event::DragMotion : {
            if (dragging_widget) {
                if (dragging_widget->handle(event)) {
                    return true;
                }
            }
            // Unhandled or no widget under mouse
            return drag_motion(event.as<DragEvent>());
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
        Widget *top = parent();
        if (top == nullptr) {
            // Drop it
            return ;
        }
        while (top->parent() != nullptr) {
            top = top->parent();
        }
        return top->repaint();
    }

    // Is window , let driver to send a paint event
    _win->repaint();
}

// Query

Rect Widget::rect() const {
    return _rect;
}
bool Widget::visible() const {
    return _visible;
}
bool Widget::is_window() const {
    return _win != nullptr;
}
bool Widget::is_root() const {
    return _parent == nullptr;
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

// Size Hint
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
    return _context->graphics_driver();
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

// Window configure
void Widget::set_window_title(u8string_view title) {
    if (is_window()) {
        _win->set_title(title.data());
    }
}
void Widget::set_window_size(int w, int h) {
    if (is_window()) {
        _win->resize(w, h);
    }
}
void Widget::set_window_position(int x, int y) {
    if (is_window()) {
        _win->move(x, y);
    }
}
void Widget::set_window_icon(const PixBuffer &icon) {
    if (is_window()) {
        _win->set_icon(icon);
    }
}
void Widget::capture_mouse(bool capture) {
    if (is_window()) {
        _win->capture_mouse(capture);
    }
}

// Common useful widgets

Button::Button(Widget *parent ) {
    if (parent != nullptr) {
        parent->add_child(this);
    }
    auto style = this->style();
    resize(style->button_width, style->button_height);
}
Button::~Button() {

}

bool Button::paint_event(PaintEvent &event) {
    BTK_UNUSED(event);

    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto &gc  = this->painter();

    // Begin draw
    auto border = rect.apply_margin(2.0f);

    // Disable antialiasing
    gc.set_antialias(false);

    Color c;

    // Background
    if (_pressed) {
        c = style->highlight;
    }
    else{
        c = style->background;
    }
    gc.set_color(c.r, c.g, c.b, c.a);
    gc.fill_rect(border);

    if (_entered && !_pressed) {
        c = style->highlight;
    }
    else{
        c = style->border;
    }

    // Border
    if (!(_flat && !_pressed && !_entered)) {
        // We didnot draw border on _flat mode
        gc.set_color(c.r, c.g, c.b, c.a);
        gc.draw_rect(border);
    }

    // Text
    if (!_text.empty()) {
        if (_pressed) {
            c = style->highlight_text;
        }
        else{
            c = style->text;
        }
        gc.set_text_align(Alignment::Center | Alignment::Middle);
        gc.set_font(font());
        gc.set_color(c.r, c.g, c.b, c.a);
        gc.draw_text(
            _text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
    }

    // Recover antialiasing
    gc.set_antialias(true);
    
    return true;
}
bool Button::mouse_press(MouseEvent &event) {
    _pressed = true;

    repaint();
    return true;
}
bool Button::mouse_release(MouseEvent &event) {
    _pressed = false;
    _clicked.emit();

    repaint();
    return true;
}
bool Button::mouse_enter(MotionEvent &event) {
    _entered = true;
    _howered.emit();
    repaint();
    return true;
}
bool Button::mouse_leave(MotionEvent &event) {
    _entered = false;
    _pressed = false;
    repaint();
    return true;
}

Size Button::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}

// Label
Label::Label(Widget *wi, u8string_view txt) : Widget(wi) {
    _layout.set_font(font());
    _layout.set_text(txt);

    // Try measure text
    auto size = size_hint();
    if (size.is_valid()) {
        resize(size.w, size.h);
    }
}
Label::~Label() {}

void Label::set_text(u8string_view txt) {
    _layout.set_text(txt);
}
bool Label::paint_event(PaintEvent &) {
    auto border = this->rect().cast<float>();
    auto style = this->style();
    auto &painter = this->painter();

    painter.set_font(font());
    painter.set_text_align(_align);
    painter.set_color(style->text);

    painter.set_scissor(border);
    painter.draw_text(
        _layout,
        border.x,
        border.y + border.h / 2
    );
    painter.reset_scissor();

    return true;
}
Size Label::size_hint() const {
    auto size = _layout.size();
    if (size.is_valid()) {
        // Add margin
        return Size(size.w + 2, size.h + 2);
    }
    return size;
}


// ProgressBar
ProgressBar::ProgressBar(Widget *parent) {
    if (parent) {
        parent->add_child(this);
    }
}
ProgressBar::~ProgressBar() {

}

void ProgressBar::set_range(int64_t min, int64_t max) {
    _min = min;
    _max = max;
    _range_changed.emit();
    repaint();
}
void ProgressBar::set_value(int64_t value) {
    value = clamp(value, _min, _max);
    _value = value;
    _value_changed.emit();
    repaint();
}
void ProgressBar::set_direction(Direction direction) {
    _direction = direction;
    repaint();
}
void ProgressBar::set_text_visible(bool visible) {
    _text_visible = visible;
    repaint();
}
void ProgressBar::reset() {
    set_value(_min);

    // TODO : reset animation
}
bool ProgressBar::paint_event(PaintEvent &) {
    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect.apply_margin(2.0f);

    gc.set_antialias(false);

    // Background
    gc.set_color(style->background);
    gc.fill_rect(border);

    // Progress
    gc.set_color(style->highlight);

    auto bar = border;

    switch(_direction) {
        case LeftToRight : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            break;
        }
        case RightToLeft : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            bar.x += border.w - bar.w;
            break;
        }
        case TopToBottom : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            break;
        }
        case BottomToTop : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            bar.y += border.h - bar.h;
            break;
        }
    }

    gc.fill_rect(bar);

    if (_text_visible) {
        int percent = (_value - _min) * 100 / (_max - _min);
        auto text = u8string::format("%d %%", percent);

        gc.set_text_align(Alignment::Center | Alignment::Middle);
        gc.set_font(font());

        gc.set_color(style->text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );

        gc.set_scissor(bar);
        gc.set_color(style->highlight_text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.reset_scissor();
    }


    // Border
    gc.set_color(style->border);
    gc.draw_rect(border);

    // End
    gc.set_antialias(true);
    
    return true;
}

BTK_NS_END