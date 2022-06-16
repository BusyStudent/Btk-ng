#pragma once

#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class Style {
    public:
        Color text;
        Color window;
        Color background;
        Color selection;
        Color highlight;
        Color border;
        // Button
        Color button;
        int   button_width; //< Default button width
        int   button_height; //< Default button height
};

BTKAPI void StyleBreeze(Style *style);

BTK_NS_END