// Common useful widgets

#pragma once

#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widget.hpp>
#include <Btk/layout.hpp>

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

// Dialog
class Dialog : public Widget {
    public:
};

// ListView MVC
class ItemModel {

};

BTK_NS_END