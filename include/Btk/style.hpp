#pragma once

#include <Btk/painter.hpp>
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
        Color highlight_text;

        // Button
        Color button_boarder;
        Color button_background;
        Color button_hovered_border;
        Color button_hovered_background;
        Color button_pressed_border;
        Color button_pressed_background;

        int   button_width; //< Default button width
        int   button_height; //< Default button height
        float button_radius; //< Default button radius

        // ProgressBar
        int   progressbar_width; //< Default progressbar width
        int   progressbar_height; //< Default progressbar height

        // Font
        Font  font;
};

BTKAPI void StyleBreeze(Style *style);

BTK_NS_END