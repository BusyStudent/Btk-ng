#pragma once

#include <Btk/painter.hpp>
#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class PaletteImpl;

class BTKAPI Palette {
    public:
        Palette();
        Palette(const Palette &);
        ~Palette();

        enum Group : uint8_t {
            Active,
            Inactive,
            Disable,
            MaxGroup
        };
        enum Role  : uint8_t {
            Window,
            Button,
            Input,
            Border,
            Hightlight,

            Text,
            PlaceholderText,
            HightlightedText,

            MaxRole,
        };
        auto copy_group(const Palette &p, Group dst, Group src) -> void;
        auto set_brush(Group gp, Role r, const Brush &brhs) -> void;
        auto set_color(Group gp, Role r, const GLColor &cl) -> void;
        auto brush_at(Group gp, Role r) const -> const Brush &;
        auto color_at(Group gp, Role r) const -> GLColor;
        auto empty()                    const -> bool;

        auto to_string()    const -> u8string;
        auto load(u8string_view conf) -> void;

        auto operator =(const Palette &) -> Palette &;
    private:
        void begin_mut();

        PaletteImpl *priv = nullptr;
};

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

        int   radio_button_circle_pad;
        int   radio_button_circle_r; //< Default radio button box width
        int   radio_button_circle_inner_pad;
        int   radio_button_text_pad;
        // ProgressBar
        int   progressbar_width; //< Default progressbar width
        int   progressbar_height; //< Default progressbar height

        // Slider
        int   slider_width; //< Default slider width
        int   slider_height; //< Default slider height

        // Margin
        float margin; //< Default margin

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
BTKAPI void PaletteBreeze(Palette *palette);

BTK_NS_END