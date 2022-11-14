// Common useful widgets

#pragma once

#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

// Frame 
class BTKAPI Frame : public Widget {
    public:
        enum FrameStyle : uint8_t {
            NoFrame,
            VLine,
            HLine,
            Box,
        };

        Frame(Widget *parent = nullptr);
        ~Frame();
        
        bool paint_event(PaintEvent &) override;
    private:
        FrameStyle _frame_style = Box;
        float      _line_width  = 1.0f;
};

// ProgressBar
class BTKAPI ProgressBar : public Widget {
    public:
        ProgressBar(Widget *parent = nullptr);
        ~ProgressBar();

        void set_value(int64_t value);
        void set_range(int64_t min, int64_t max);
        void set_direction(direction_t d);
        void set_text_visible(bool visible);
        void reset(); //< Reset to min value

        int64_t value() const {
            return _value;
        }

        bool paint_event(PaintEvent &event) override;

        BTK_EXPOSE_SIGNAL(_value_changed);
        BTK_EXPOSE_SIGNAL(_range_changed);
    private:
        int64_t _min = 0;
        int64_t _max = 100;
        int64_t _value = 0;

        u8string  _format = "%p %";

        Direction _direction = LeftToRight;
        bool      _text_visible = false;

        Signal<void()> _value_changed;
        Signal<void()> _range_changed;
};

// Dialog
class Dialog : public Widget {
    public:
};

// ListView MVC
class ItemModel {

};

BTK_NS_END