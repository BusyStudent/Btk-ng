#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

// Slider & ScrollBar
class BTKAPI AbstractSlider : public Widget {
    public:
        using Widget::Widget;

        void set_range(int64_t min, int64_t max);
        void set_value(int64_t value);
        
        void set_orientation(Orientation ori);
        void set_page_step(uint64_t step);
        void set_line_step(uint64_t step);
        void set_tracking(bool tracking);

        int64_t value() const noexcept {
            return _value;
        }

        BTK_EXPOSE_SIGNAL(_value_changed);
        BTK_EXPOSE_SIGNAL(_range_changed);
        BTK_EXPOSE_SIGNAL(_slider_released);
        BTK_EXPOSE_SIGNAL(_slider_pressed);
        BTK_EXPOSE_SIGNAL(_slider_moved);
    protected:
        int64_t _min = 0;
        int64_t _max = 100;
        int64_t _value = 0;

        uint64_t _page_step = 1;
        uint64_t _single_step = 1;

        Orientation _orientation = Horizontal;

        Signal<void()> _value_changed;
        Signal<void()> _range_changed;
        Signal<void()> _slider_released;
        Signal<void()> _slider_pressed;
        Signal<void()> _slider_moved;
};

class BTKAPI Slider : public AbstractSlider {
    public:
        Slider(Widget *parent = nullptr, Orientation ori = Horizontal);
        Slider(Orientation ori) : Slider(nullptr, ori) {};
        ~Slider();

        bool paint_event(PaintEvent &) override;

        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool mouse_wheel(WheelEvent &event) override;

        bool drag_begin(DragEvent &event) override;
        bool drag_motion(DragEvent &event) override;
        bool drag_end(DragEvent &event) override;

        Size size_hint() const override;

    private:
        void  proc_mouse(Point where);
        FRect content_rect() const;
        FRect bar_rect()     const;

        bool  _dragging = false;
        bool  _pressed = false;
        bool  _hovered = false;
};

class BTKAPI ScrollBar : public AbstractSlider {
    public:
        ScrollBar(Widget *parent = nullptr, Orientation ori = Horizontal);
        ScrollBar(Orientation ori) : ScrollBar(nullptr, ori) {}
        ~ScrollBar();

        bool paint_event(PaintEvent &) override;

        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_motion(MotionEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;
        bool mouse_wheel(WheelEvent &event) override;

        bool drag_begin(DragEvent &event) override;
        bool drag_motion(DragEvent &event) override;
        bool drag_end(DragEvent &event) override;

        Size size_hint() const override;

    private:
        void  proc_mouse(Point where);
        FRect content_rect() const;
        FRect bar_rect()     const;

        bool  _dragging = false;
        bool  _pressed = false;
        bool  _hovered = false;

        float _length_ratio = 0.2f;
        float _bar_offset   = 0.0f; //< Offset used in drag
};

BTK_NS_END