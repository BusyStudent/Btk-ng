#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

// Slider & ScrollBar
class BTKAPI AbstractSlider : public Widget {
    public:
        using Widget::Widget;

        void set_range(double min, double max);
        void set_value(double value);
        
        void set_orientation(Orientation ori);
        void set_page_step(double step);
        void set_line_step(double step);
        void set_tracking(bool tracking);

        double value() const noexcept {
            return _value;
        }
        double page_step() const noexcept {
            return _page_step;
        }
        double single_step() const noexcept {
            return _single_step;
        }

        BTK_EXPOSE_SIGNAL(_value_changed);
        BTK_EXPOSE_SIGNAL(_range_changed);
        BTK_EXPOSE_SIGNAL(_slider_released);
        BTK_EXPOSE_SIGNAL(_slider_pressed);
        BTK_EXPOSE_SIGNAL(_slider_moved);
    protected:
        double _min = 0;
        double _max = 100;
        double _value = 0;

        double _page_step = 10;
        double _single_step = 1;

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

        float _bar_offset   = 0.0f; //< Offset used in drag
};

// ScrollArea
// ----------
//          /
//          /
// ========[]
// ----------
class ScrollArea : public Widget {
    public:
        ScrollArea(Widget *parent = nullptr);
        ~ScrollArea();

        bool mouse_wheel(WheelEvent &) override;
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;

        void set_viewport(Widget *viewport);
        
        Widget viewport() const {
            return _viewport;
        }
    private:
        void setup_scroll();
        void vscroll_moved();
        void hscroll_moved();

        Point   _offset = {0, 0};
        Widget *_viewport = nullptr;
        ScrollBar _vscroll {this, Vertical};
        ScrollBar _hscroll {this, Horizontal};
};

BTK_NS_END