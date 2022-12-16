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
            Window, //< Background color
            Button, //< Button background color
            Input,  //< TextInput / Edit background color
            Border, //< All border color
            Hightlight,

            Text,
            PlaceholderText,
            HightlightedText,

            Shadow,

            MaxRole,
        };
        auto copy_group(const Palette &p, Group dst, Group src) -> void;
        auto set_brush(Group gp, Role r, const Brush &brhs) -> void;
        auto set_color(Group gp, Role r, const GLColor &cl) -> void;
        auto brush_at(Group gp, Role r) const -> const Brush &;
        auto color_at(Group gp, Role r) const -> GLColor;
        auto empty()                    const -> bool;

        auto operator =(const Palette &) -> Palette &;


        // Current color group access
        auto set_current_group(Group gp) -> void {
            group = gp;
        }

        // Access
        auto window() const -> const Brush & {
            return brush_at(group, Window);
        }
        auto button() const -> const Brush & {
            return brush_at(group, Button);
        }
        auto input() const -> const Brush & {
            return brush_at(group, Input);
        }
        auto border() const -> const Brush & {
            return brush_at(group, Border);
        }
        auto hightlight() const -> const Brush & {
            return brush_at(group, Hightlight);
        }
        
        auto text() const -> const Brush & {
            return brush_at(group, Text);
        }
        auto placeholder_text() const -> const Brush & {
            return brush_at(group, PlaceholderText);
        }
        auto hightlighted_text() const -> const Brush & {
            return brush_at(group, HightlightedText);
        }

        auto shadow() const -> const Brush & {
            return brush_at(group, Shadow);
        }
        
    private:
        void begin_mut();

        PaletteImpl *priv = nullptr;
        Group        group = Inactive; //< Current role
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

        // ScrollBar
        int   scrollbar_width;
        int   scrollbar_height;
        
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