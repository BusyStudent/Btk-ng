#pragma once

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

BTK_NS_END