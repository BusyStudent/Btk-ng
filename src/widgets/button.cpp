#include "build.hpp"

#include <Btk/widgets/button.hpp>
#include <Btk/event.hpp>


BTK_NS_BEGIN


Button::Button(Widget *parent, u8string_view txt) : AbstractButton(parent) {
    auto style = this->style();
    resize(style->button_width, style->button_height);

    if (!txt.empty()) {
        set_text(txt);
    }
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
    gc.set_antialias(true);

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

    if (under_mouse() && !_pressed) {
        c = style->highlight;
    }
    else{
        c = style->border;
    }

    // Border
    if (!(_flat && !_pressed && !under_mouse())) {
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

        gc.push_scissor(border);
        gc.draw_text(
            _text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.pop_scissor();
    }

    // Recover antialiasing
    gc.set_antialias(true);
    
    return true;
}
bool Button::mouse_press(MouseEvent &event) {
    if (event.button() == MouseButton::Left) {
        _pressed = true;

        repaint();
    }
    return true;
}
bool Button::mouse_release(MouseEvent &event) {
    if (_pressed) {
        _clicked.emit();
        _pressed = false;
        
        repaint();
    }
    return true;
}
bool Button::mouse_enter(MotionEvent &event) {
    _howered.emit();
    repaint();
    return true;
}
bool Button::mouse_leave(MotionEvent &event) {
    _pressed = false;
    repaint();
    return true;
}

Size Button::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}

RadioButton::RadioButton(Widget *p) : AbstractButton(p) {
    resize(size_hint());
}
RadioButton::~RadioButton() {

}
bool RadioButton::paint_event(PaintEvent &) {
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect().apply_margin(style->margin);

    // Clac the center circle (Left, Middle)
    float cx = border.x + style->radio_button_circle_pad + style->radio_button_circle_r / 2;
    float cy = border.y + border.h / 2;

    gc.set_antialias(true);

    // Draw background
    gc.set_color(style->background);
    gc.fill_circle(cx, cy, style->radio_button_circle_r);
    if (_checked) {
        // Draw inner box
        gc.set_color(style->highlight);
        gc.fill_circle(cx, cy, style->radio_button_circle_r);
        gc.set_color(style->background);
        gc.fill_circle(cx, cy, style->radio_button_circle_r - style->radio_button_circle_inner_pad);
    }
    else {
        // We didnot draw border at checked
        if (under_mouse()) {
            gc.set_color(style->highlight);
        }
        else {
            gc.set_color(style->border);
        }
        gc.draw_circle(cx, cy, style->radio_button_circle_r);
    }

    // Draw text if exist
    if (!_text.empty()) {
        gc.push_scissor(border);
        gc.set_font(font());
        gc.set_text_align(Alignment::Left | Alignment::Middle);
        gc.set_color(style->text);
        gc.draw_text(
            _text,
            cx + style->radio_button_circle_r + style->radio_button_text_pad,
            cy
        );

        gc.pop_scissor();
    }

    return true;
}
bool RadioButton::mouse_press(MouseEvent &event) {
    _pressed = true;

    repaint();
    return true;
}
bool RadioButton::mouse_release(MouseEvent &event) {
    _pressed = false;
    if (!_checked) {
        _checked = true;
        _clicked.emit();
    }
    repaint();
    return true;
}
bool RadioButton::mouse_enter(MotionEvent &event) {
    _howered.emit();
    repaint();
    return true;
}
bool RadioButton::mouse_leave(MotionEvent &event) {
    _pressed = false;
    repaint();
    return true;
}
Size RadioButton::size_hint() const {
    auto style = this->style();
    return Size(style->button_width, style->button_height);
}

BTK_NS_END