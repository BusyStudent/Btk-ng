#include "build.hpp"

#include <Btk/widgets/slider.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

// AbstractSlider
void AbstractSlider::set_value(double value) {
    value = clamp(value, _min, _max);
    _value = value;
    _value_changed.emit();
    repaint();
}
void AbstractSlider::set_range(double min, double max) {
    _min = min;
    _max = max;
    _value = clamp(_value, _min, _max);
    _range_changed.emit();
    _value_changed.emit();
    repaint();
}
void AbstractSlider::set_single_step(double step) {
    _single_step = step;
    repaint();
}
void AbstractSlider::set_page_step(double step) {
    _page_step = step;
    repaint();
}
void AbstractSlider::set_orientation(Orientation ori) {
    _orientation = ori;
    repaint();
}

// Slider
// TODO : This widget is hardcoded, so we have to think how to make it more flexible.
Slider::Slider(Widget *p, Orientation ori) : AbstractSlider(p) {
    set_orientation(ori);
    resize(size_hint());
}
Slider::~Slider() {

}
Size Slider::size_hint() const {
    auto s = style();
    if (_orientation == Horizontal) {
        return Size(100, s->slider_height);
    }
    else {
        return Size(s->slider_width, 100);
    }
}
bool Slider::paint_event(PaintEvent &) {
    auto content = content_rect();
    auto bar     = bar_rect();
    auto s       = style();
    auto &p = painter();

    p.set_antialias(window_dpi().x > 96.0f);

    // Fill the content rect
    p.set_brush(palette().input());
    p.fill_rect(content);
    p.set_brush(palette().border());
    p.draw_rect(content);

    // Fill the content rect by the value
    p.set_brush(palette().hightlight());
    if (_orientation == Horizontal) {
        content.w = ((content.w - bar.w) * _value) / (_max - _min);
    }
    else {
        content.h = ((content.h - bar.h) * _value) / (_max - _min);
    }
    p.fill_rect(content);

    // Draw the bar
    if (_pressed) {
        p.set_brush(palette().hightlight());
    }
    else {
        p.set_brush(palette().input());
    }
    p.fill_rect(bar);

    if (_hovered) {
        p.set_brush(palette().hightlight());
    }
    else {
        p.set_brush(palette().border());
    }
    p.draw_rect(bar);

    return true;
}
bool Slider::mouse_press(MouseEvent &event) {
    if (event.button() == MouseButton::Left) {
        _pressed = true;
        proc_mouse(event.position());

        // Value update, & trigger it
        _slider_moved.emit();
    }
    return true;
}
bool Slider::mouse_release(MouseEvent &event) {
    _pressed = false;
    repaint();
    return true;
}
bool Slider::mouse_enter(MotionEvent &event) {
    _hovered = true;
    repaint();
    return true;
}
bool Slider::mouse_leave(MotionEvent &event) {
    if (!_dragging) {
        // Is dragging, we don't reset the pressed state
        _pressed = false;
    }
    _hovered = false;
    repaint();
    return true;
}
bool Slider::mouse_wheel(WheelEvent &event) {
    set_value(_value + event.y() * _page_step);

    // Value update, & trigger it
    _slider_moved.emit();
    return true;
}

bool Slider::drag_begin(DragEvent &event) {
    _slider_pressed.emit();
    if (_pressed) {
        _dragging = true;
        repaint();
        return true;
    }
    return false;
}
bool Slider::drag_motion(DragEvent &event) {
    // Update the value by it
    proc_mouse(event.position());

    // Value update, & trigger it
    _slider_moved.emit();
    return true;
}
bool Slider::drag_end(DragEvent &event) {
    _slider_released.emit();
    _dragging = false;
    _pressed  = false;
    repaint();
    return true;
}

FRect Slider::content_rect() const {
    auto r = FRect(0, 0, size()).apply_margin(style()->margin);

    FPoint center;
    center.x = r.x + r.w / 2;
    center.y = r.y + r.h / 2;

    // Get the size of the bar
    float w = 5;
    float h = 5;

    if (_orientation == Horizontal) {
        r.y = center.y - h / 2;
        r.h = h;
    }
    else {
        r.x = center.x - w / 2;
        r.w = w;
    }
    return r;
}
FRect Slider::bar_rect() const {
    // Calc the bar rectangle
    FRect prev = content_rect();
    FRect r = prev;
    if (_orientation == Horizontal) {
        r.w = 7;
        r.h = 18;
        r.x = r.x + ((prev.w - r.w) * (_value - _min)) / (_max - _min);
        r.y = r.y - r.h / 2 + prev.h / 2; //< Center the bar
    }
    else {
        r.h = 7;
        r.w = 18;
        r.y = r.y + ((prev.h - r.h) * (_value - _min)) / (_max - _min);
        r.x = r.x - r.w / 2 + prev.w / 2; //< Center the bar
    }
    return r;
}
void  Slider::proc_mouse(Point where) {
    // Calc the value
    auto rect = content_rect();
    auto bar  = bar_rect();
    double v;

    if (_orientation == Horizontal) {
        v = (where.x - rect.x) * (_max - _min) / (rect.w - bar.w);
    }
    else {
        v = (where.y - rect.y) * (_max - _min) / (rect.h - bar.h);
    }

    set_value(std::round(v));
}

// ScrollBar
ScrollBar::ScrollBar(Widget *parent, Orientation ori) : AbstractSlider(parent) {
    set_orientation(ori);
    resize(size_hint());
}
ScrollBar::~ScrollBar() {

}

Size ScrollBar::size_hint() const {
    auto s = style();
    if (_orientation == Horizontal) {
        return Size(100, s->scrollbar_height);
    }
    else {
        return Size(s->scrollbar_width, 100);
    }
}
bool ScrollBar::paint_event(PaintEvent &) {
    auto content = content_rect();
    auto bar     = bar_rect();
    auto s       = style();
    auto &p = painter();

    Color color = Color(144, 144, 144);
    if (_pressed || _dragging) {
        // color = style()->highlight;
    }

    if (_pressed || _hovered) {
        // color.a = 200;
        color.a = 255;
    }
    // else if (_hovered) {
    //     color.a = 255 / 2;
    // }
    else {
        // color.a = 255;
        color.a = 220;
    }

    p.set_antialias(true);
    p.set_color(color);
    p.fill_rounded_rect(bar, 5);
    return true;
}
bool ScrollBar::mouse_press(MouseEvent &event) {
    if (event.button() == MouseButton::Left) {
        _pressed = true;

        if (!bar_rect().contains(event.position())) {
            proc_mouse(event.position());
        }
        repaint();
    }
    return true;
}
bool ScrollBar::mouse_release(MouseEvent &event) {
    _pressed = false;
    repaint();
    return true;
}
bool ScrollBar::mouse_enter(MotionEvent &event) {
    repaint();
    return true;
}
bool ScrollBar::mouse_motion(MotionEvent &event) {
    bool new_s = bar_rect().contains(event.position());
    if (new_s != _hovered) {
        _hovered = new_s;
        repaint();
    }
    return true;
}
bool ScrollBar::mouse_leave(MotionEvent &event) {
    if (!_dragging) {
        // Is dragging, we don't reset the pressed state
        _pressed = false;
    }
    _hovered = false;
    repaint();
    return true;
}
bool ScrollBar::mouse_wheel(WheelEvent &event) {
    if (_orientation == Horizontal) {
        set_value(_value + event.y() * _single_step);
    }
    else {
        set_value(_value - event.y() * _single_step);
    }
    return true;
}

bool ScrollBar::drag_begin(DragEvent &event) {
    _slider_pressed.emit();
    if (_pressed) {
        auto bar = bar_rect();
        _dragging = true;
        if (bar.contains(event.position())) {
            if (_orientation == Horizontal) {
                _bar_offset = event.position().x - bar.x;
            }
            else {
                _bar_offset = event.position().y - bar.y;
            }
        }
        else {
            _bar_offset = 0.0f;
        }

        repaint();
        return true;
    }
    return false;
}
bool ScrollBar::drag_motion(DragEvent &event) {
    // Update the value by it
    auto pos = event.position();
    pos.x -= _bar_offset;
    pos.y -= _bar_offset;
    proc_mouse(pos);
    
    // Value update, & trigger it
    _slider_moved.emit();
    return true;
}
bool ScrollBar::drag_end(DragEvent &event) {
    _slider_released.emit();
    _dragging = false;
    _pressed  = false;
    repaint();
    return true;
}

FRect ScrollBar::content_rect() const {
    FRect border = FRect(0, 0, size()).apply_margin(style()->margin);
    return border;
}
FRect ScrollBar::bar_rect() const {
    FRect border = FRect(0, 0, size()).apply_margin(style()->margin);
    FRect bar;
    double rng = _max - _min;

    // 1.0 - works better
    double len_ratio = 1.0 - (rng + _single_step) / (rng + _single_step + _page_step);

    if (_orientation == Horizontal) {
        bar.w = border.w * len_ratio;
        bar.h = border.h;

        bar.x = border.x + ((border.w - bar.w) * (_value - _min)) / rng;
        bar.y = border.y;
    }
    else {
        bar.h = border.h * len_ratio;
        bar.w = border.w;

        bar.y = border.y + ((border.h - bar.h) * (_value - _min)) / rng;
        bar.x = border.x;

    }
    return bar;
}
void  ScrollBar::proc_mouse(Point where) {
    // Calc the value
    auto rect = content_rect();
    auto bar  = bar_rect();
    double v;

    if (_orientation == Horizontal) {
        v = (where.x - rect.x) * (_max - _min) / (rect.w - bar.w);
    }
    else {
        v = (where.y - rect.y) * (_max - _min) / (rect.h - bar.h);
    }

    set_value(std::round(v));
}

// ScrollArea
ScrollArea::ScrollArea(Widget *w) : Widget(w) {
    _vscroll.signal_slider_moved().connect(&ScrollArea::vscroll_moved, this);
    _hscroll.signal_slider_moved().connect(&ScrollArea::hscroll_moved, this);
}
ScrollArea::~ScrollArea() {}

bool ScrollArea::resize_event(ResizeEvent &event) {
    auto r = FRect(0, 0, size());

    // _hscroll.hide();
    if (_viewport) {
        setup_scroll();
    }

    // Calc conert
    int h = r.h;
    int w = r.w;
    if (_hscroll.visible()) {
        h = r.h - _hscroll.height();
    }
    if (_vscroll.visible()) {
        w = r.w - _vscroll.width();
    }

    _vscroll.move(r.x + r.w - _vscroll.width(), r.y);
    _vscroll.resize(_vscroll.width(), h);

    _hscroll.move(r.x, r.y + r.h - _hscroll.height());
    _hscroll.resize(w, _hscroll.height());

    return true;
}
bool ScrollArea::move_event(MoveEvent &event) {
    auto r = FRect(0, 0, size());

    _vscroll.move(r.x + r.w - _vscroll.width(), r.y);
    _hscroll.move(r.x, r.y + r.h - _hscroll.height());

    if (_viewport) {
        _viewport->move(r.x, r.y);
    }
    return true;
}
bool ScrollArea::mouse_wheel(WheelEvent &event) {
    if (_vscroll.visible()) {
        return _vscroll.handle(event);
    }
    if (_hscroll.visible()) {
        return _hscroll.handle(event);
    }
    return false;
}
void ScrollArea::set_viewport(Widget *w) {
    auto cb = [](Object *obj, Event &event, void *self) {
        if (event.type() == Event::Resized) {
            static_cast<ScrollArea*>(self)->setup_scroll();
        }
        return false;
    };
    if (_viewport) {
        _viewport->del_event_filter(cb, this);
        remove_child(_viewport);
    }
    _viewport = w;
    if (_viewport) {
        _viewport->add_event_filter(cb, this);
        add_child(_viewport);
        setup_scroll();
        _viewport->lower();
    }

    repaint();
}
void ScrollArea::setup_scroll() {
    auto vrect = _viewport->rect();
    if (vrect.w <= width()) {
        // Disable _hscroll
        _hscroll.hide();
    }
    else {
        _hscroll.show();
        _hscroll.set_range(0, vrect.w - width());
        _hscroll.set_value(0);
    }

    if (vrect.h <= height()) {
        _vscroll.hide();
    }
    else {
        _vscroll.show();
        _vscroll.set_range(0, vrect.h - height());
        _hscroll.set_value(0);
    }
}
void ScrollArea::vscroll_moved() {
    if (_viewport) {
        _viewport->move(_viewport->x(), y() - _vscroll.value());
    }
}
void ScrollArea::hscroll_moved() {
    if (_viewport) {
        _viewport->move(x() - _hscroll.value(), _viewport->y());
    }
}

BTK_NS_END