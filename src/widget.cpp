#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

struct WidgetImpl {
    ~WidgetImpl();

    UIContext  *context;//< Pointer to UIContext
    Widget     *parent     = nullptr;//< Parent widget
    Style      *style      = nullptr;//< Pointer to style
    WindowFlags flags      = WindowFlags::Resizable;//< Window flags
    bool        visible    = true;//< Visibility
    std::list<Widget *> children;//< Child widgets
    std::list<Widget *>::iterator in_child_iter = {};//< Child iterator

    Rect        rect = {0, 0, 0, 0};//< Rectangle

    window_t    win     = {};//< Window handle
    gcontext_t  gc      = {};//< Graphics context
    bool        is_vgc  = false;//< Can be cast to vgcontext_t

    GraphicsDriver *driver() {
        return context->graphics_driver();
    }
};

WidgetImpl::~WidgetImpl() {
    delete gc;
    delete win;
}

Widget::Widget() {
    priv = new WidgetImpl;
    priv->context = GetUIContext();
    priv->style   = &priv->context->style;

    BTK_ASSERT(priv->context);
}
Widget::~Widget() {
    if(priv->in_child_iter != std::list<Widget *>::iterator{}){
        priv->parent->priv->children.erase(priv->in_child_iter);
    }
    // Clear children
    for(auto w : priv->children) {
        // Set iter to {}
        w->priv->in_child_iter = std::list<Widget *>::iterator{};
        delete w;
    }

    delete priv;
}

void Widget::set_visible(bool v) {
    if (priv->parent == nullptr) {
        //Top level widget
        if (priv->win == nullptr) {
            Size size = priv->rect.size();
            // Try to use size hint
            if (size.w == 0 || size.h == 0) {
                size = size_hint();
            }
            // Still no size, use default
            if (size.w == 0 || size.h == 0) {
                size = {100, 100};
                priv->rect.w = size.w;
                priv->rect.h = size.h;
            }

            priv->win = priv->driver()->window_create(
                nullptr,
                size.w,
                size.h,
                priv->flags
            );
            priv->win->bind_widget(this);
            priv->gc = priv->win->gc_create();
        }
        priv->win->show(v);
    }
    priv->visible = v;
    Event event(v ? Event::Show : Event::Hide);
    handle(event);
}
void Widget::show() {
    set_visible(true);
}
void Widget::hide() {
    set_visible(false);
}
void Widget::resize(int w, int h) {
    int old_w = priv->rect.w;
    int old_h = priv->rect.h;
    priv->rect.w = w;
    priv->rect.h = h;
    if (priv->win != nullptr) {
        priv->win->resize(w, h);
    }
    ResizeEvent event;
    event.set_new_size(w, h);
    event.set_old_size(old_w, old_h);
    handle(event);
}
bool Widget::handle(Event &event) {
    // printf("Widget::handle(%d)\n", event.type());
    switch (event.type()) {
        case Event::Paint : {
            if (priv->win) {
                auto style = priv->style;
                auto c     = style->window;
                priv->gc->set_color(c.r, c.g, c.b, c.a);
                priv->gc->begin();
                priv->gc->clear();
            }
            // Paint children first
            for(auto w : priv->children) {
                if (!w->priv->visible) {
                    continue;
                }
                w->handle(event);
            }

            //Let children handle it
            paint_event(static_cast<PaintEvent &>(event));
            if (priv->win) {
                priv->gc->end();
            }
            break;
        }
        case Event::Resized : {
            auto &e = static_cast<ResizeEvent &>(event);
            if (is_window()) {
                // Size w, h
                priv->rect.x = 0;
                priv->rect.y = 0;
                priv->rect.w = e.width();
                priv->rect.h = e.height();
            }
            break;
        }
        case Event::KeyPress : {
            return key_press(static_cast<KeyEvent &>(event));
        }
        case Event::KeyRelease : {
            return key_release(static_cast<KeyEvent &>(event));
        }
        case Event::MousePress : {
            return mouse_press(static_cast<MouseEvent &>(event));
        }
        case Event::MouseRelease : {
            return mouse_release(static_cast<MouseEvent &>(event));
        }
        case Event::MouseEnter : {
            return mouse_enter(static_cast<MotionEvent &>(event));
        }
        case Event::MouseLeave : {
            return mouse_leave(static_cast<MotionEvent &>(event));
        }
        case Event::MouseMotion : {
            return mouse_motion(static_cast<MotionEvent &>(event));
        }
        default: {
            break;
        }
    }
    // Unhandled
    return false;
}
void Widget::repaint() {
    // TODO Check is GUI Thread
    if (!is_window()) {
        Widget *top = parent();
        while (top->parent() != nullptr) {
            top = top->parent();
        }
        return top->repaint();
    }

    if (false) {
        PaintEvent event;
        event.set_widget(this);
        return priv->context->send_event(event);
    }

    PaintEvent event;
    handle(event);
}

// Query

Rect Widget::rect() const {
    return priv->rect;
}
bool Widget::visible() const {
    return priv->visible;
}
bool Widget::is_window() const {
    return priv->win != nullptr;
}
Widget *Widget::parent() const {
    return priv->parent;
}
Style  *Widget::style() const {
    return priv->style;
}

// Size Hint
Size Widget::size_hint() const {
    return Size(0, 0);
}

window_t Widget::winhandle() const {
    if (priv->parent == nullptr) {
        return priv->win;
    }
    return nullptr;
}
gcontext_t Widget::gc() const {
    if (priv->gc == nullptr) {
        // To top level widget
        auto prev = priv->parent;
        while (prev->priv->parent != nullptr) {
            prev = prev->priv->parent;
        }
        priv->gc = prev->priv->gc;
    }
    return priv->gc;
}

// Child widgets
Widget *Widget::child_at(int x,int y) const {
    for(auto w : priv->children) {
        if (w->rect().contains(x, y)) {
            return w;
        }
    }
    return nullptr;
}

// Common useful widgets

Button::Button() {
    auto style = this->style();
    resize(style->button_width, style->button_height);
}
Button::~Button() {

}

bool Button::paint_event(PaintEvent &event) {
    BTK_UNUSED(event);

    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto gc = this->gc();

    // Begin draw
    auto boarder = rect.apply_margin(2.0f);

    Color c;

    // Background
    if (_pressed) {
        c = style->highlight;
    }
    else{
        c = style->background;
    }
    gc->set_color(c.r, c.g, c.b, c.a);
    gc->fill_rects(&boarder,1);

    // if (_entered) {
    //     c = style->highlight;
    // }
    // else{
    c = style->border;
    // }

    // Border
    gc->set_color(c.r, c.g, c.b, c.a);
    gc->draw_rects(&boarder,1);
    
    return true;
}
bool Button::mouse_press(MouseEvent &event) {
    _pressed = true;
    _clicked.emit();

    repaint();
    return true;
}
bool Button::mouse_release(MouseEvent &event) {
    _pressed = false;
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
    repaint();
    return true;
}

Size Button::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}


BTK_NS_END