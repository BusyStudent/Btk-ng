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

        // Slider
        int   slider_width; //< Default slider width
        int   slider_height; //< Default slider height

        // Margin
        int   margin; //< Default margin

        // Icon 
        float icon_width; //< Default icon width
        float icon_height; //< Default icon height

        // Window
        int   window_width; //< Default window width
        int   window_height; //< Default window height
        
        // Font
        Font  font;

        // Shadow
        Brush shadow;
        float shadow_offset_x;
        float shadow_offset_y;
        float shadow_radius;
};

BTKAPI void StyleBreeze(Style *style);

BTK_NS_END