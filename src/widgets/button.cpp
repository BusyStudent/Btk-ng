#include "build.hpp"

#include <Btk/widgets/button.hpp>
#include <Btk/event.hpp>


BTK_NS_BEGIN

// AbstractButton
AbstractButton::AbstractButton(Widget *parent) : Widget(parent) {
    auto s = style();
    _icon_size.w = s->icon_width;
    _icon_size.h = s->icon_height;
}
void AbstractButton::set_flat(bool flat) {
    _flat = flat;
    repaint();
}
void AbstractButton::set_icon(const PixBuffer &icon) {
    if (icon.empty()) {
        _has_icon = false;
        return;
    }
    else {
        _icon.set_image(icon);
        _has_icon = true;
    }
    repaint();
}
void AbstractButton::set_icon_size(const Size &size) {
    _icon_size = size;
    if (!_icon_size.is_valid()) {
        auto s = style();
        _icon_size.w = s->icon_width;
        _icon_size.h = s->icon_height;
    }
    repaint();
}
void AbstractButton::set_text(u8string_view us) {     
    _text = us;
    _textlay.set_font(font());
    _textlay.set_text(_text);
    repaint();
}
void AbstractButton::set_text_align(Alignment alig) {
    _textalign = alig;
    repaint();
}
void AbstractButton::paint_icon_text(const FRect &border) {
    auto &gc = painter();

    // Text & Icon
    FRect icon_rect = {0.0f, 0.0f, 0.0f, 0.0f};

    if (_icon.type() == BrushType::Bitmap) {
        // Has Icon
        // Icon at center of this button, calc it
        float center_y = border.y + border.h / 2;

        float y = center_y - float(_icon_size.h) / 2;
        float x = border.x + (y - border.y);
        icon_rect = FRect(x, y, _icon_size.w, _icon_size.h);

        gc.set_brush(_icon);
        gc.fill_rect(icon_rect);
    }
    if (!_text.empty()) {
        if (_pressed || !has_focus() || under_mouse()) {
            // Normal text color on nofocus or under pressed
            // c = style->highlight_text;
            gc.set_brush(palette().text());
        }
        else{
            // c = style->text;
            gc.set_brush(palette().hightlighted_text());
        }
        gc.set_text_align(Alignment::Left | Alignment::Top);
        // gc.set_color(c.r, c.g, c.b, c.a);

        // Text border
        float icon_width = 0.0f;
        if (icon_rect.x != 0.0f) {
            icon_width = icon_rect.w + (icon_rect.x - border.x) * 2;
        }

        auto tborder = border;
        tborder.x += icon_width;
        tborder.w -= icon_width;

        gc.push_scissor(tborder);

        FRect result = tborder.align_object(_textlay.size(), _textalign);

        gc.draw_text(
            _textlay,
            result.x,
            result.y
        );
        gc.pop_scissor();
    }
}
bool AbstractButton::change_event(ChangeEvent &event) {
    if (event.type() == Event::FontChanged) {
        _textlay.set_font(font());

        repaint();
    }
    return true;
}
// Button
Button::Button(Widget *parent, u8string_view txt) : AbstractButton(parent) {
    _textalign = AlignMiddle | AlignCenter;
    auto style = this->style();
    resize(style->button_width, style->button_height);
    set_focus_policy(FocusPolicy::Mouse);

    if (!txt.empty()) {
        set_text(txt);
    }
}
Button::~Button() {

}

bool Button::paint_event(PaintEvent &event) {
    BTK_UNUSED(event);

    auto rect = FRect(0, 0, size());
    auto style = this->style();
    auto &gc  = this->painter();

    // Begin draw
    auto border = rect.apply_margin(2.0f);

    // In low dpi screens, we didnot need to antialiasing the rectangle
    gc.set_antialias(window_dpi().x > 96.0f);
    gc.push_transform();

    // Background
    if (_pressed) {
        // c = style->highlight;
        // Make a little translate
        gc.translate(0.5, 0);
        gc.set_brush(palette().button_pressed());
    }
    else if (under_mouse()) {
        // Hover
        gc.set_brush(palette().button_hovered());
    }
    else{
        // c = style->background;
        gc.set_brush(palette().button());
    }
    // gc.set_color(c.r, c.g, c.b, c.a);
    gc.fill_rect(border);

    if (under_mouse()) {
        // c = style->highlight;
        gc.set_brush(palette().hightlight());
    }
    else{
        // c = style->border;
        gc.set_brush(palette().border());
    }

    // Border
    if (!_flat) {
        // We didnot draw border on _flat mode
        // gc.set_color(c.r, c.g, c.b, c.a);
        gc.draw_rect(border);
    }

    paint_icon_text(border);

    // Recover antialiasing
    gc.set_antialias(true);
    gc.pop_transform();
    
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
bool Button::focus_gained(FocusEvent &event) {
    BTK_LOG("Button::focus_gained\n");
    set_palette_current_group(Palette::Active);
    repaint();
    return true;
}
bool Button::focus_lost(FocusEvent &event) {
    BTK_LOG("Button::focus_lost\n");
    set_palette_current_group(Palette::Inactive);
    repaint();
    return true;
}

Size Button::size_hint() const {
    auto style = this->style();
    if (!_text.empty()) {
        TextLayout lay;
        lay.set_text(_text);
        lay.set_font(font());

        auto [w, h] = lay.size();
        return Size(max<int>(w + style->margin * 2, style->button_width), style->button_height);
    }
    return Size(style->button_width, style->button_height);
}

// RadioButton
RadioButton::RadioButton(Widget *p, u8string_view txt) : AbstractButton(p) {
    resize(size_hint());

    if (!txt.empty()) {
        set_text(txt);
    }
}
RadioButton::~RadioButton() {

}
bool RadioButton::paint_event(PaintEvent &) {
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = Rect(0, 0, size()).apply_margin(style->margin);

    // Clac the center circle (Left, Middle)
    float cx = border.x + style->radio_button_circle_pad + style->radio_button_circle_r / 2;
    float cy = border.y + border.h / 2;

    gc.set_antialias(true);

    // Draw background
    gc.set_brush(palette().input());
    gc.fill_circle(cx, cy, style->radio_button_circle_r);
    if (_checked) {
        // Draw inner box
        gc.set_brush(palette().hightlight());
        gc.fill_circle(cx, cy, style->radio_button_circle_r);
        gc.set_brush(palette().input());
        gc.fill_circle(cx, cy, style->radio_button_circle_r - style->radio_button_circle_inner_pad);
    }
    else {
        // We didnot draw border at checked
        if (under_mouse()) {
            gc.set_brush(palette().hightlight());
        }
        else {
            gc.set_brush(palette().border());
        }
        gc.draw_circle(cx, cy, style->radio_button_circle_r);
    }

    // Calc the icon & text begin box
    FRect box;
    box.x = cx + style->radio_button_circle_r + style->radio_button_text_pad;
    box.y = border.y;
    box.w = border.x + border.w - box.x;
    box.h = border.h;

    paint_icon_text(box);

    // if (!_text.empty()) {
    //     gc.push_scissor(border);
    //     gc.set_font(font());
    //     gc.set_text_align(Alignment::Left | Alignment::Middle);
    //     gc.set_brush(palette().text());
    //     gc.draw_text(
    //         _textlay,
    //         cx + style->radio_button_circle_r + style->radio_button_text_pad,
    //         cy
    //     );

    //     gc.pop_scissor();
    // }

    return true;
}
bool RadioButton::mouse_press(MouseEvent &event) {
    _pressed = true;

    repaint();
    return true;
}
bool RadioButton::mouse_release(MouseEvent &event) {
    _pressed = false;
    _checked = !_checked;
    _clicked.emit();
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