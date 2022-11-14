#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/style.hpp>

BTK_NS_BEGIN

class PaletteImpl : public Refable<PaletteImpl> {
    public:
        Brush brushs[Palette::MaxGroup][Palette::MaxRole];
};

COW_MUT_IMPL(Palette);

Palette::Palette() {
    priv = nullptr;
}
Palette::Palette(const Palette &p) {
    priv = COW_REFERENCE(p.priv);
}
Palette::~Palette() {
    COW_RELEASE(priv);
}
Palette &Palette::operator =(const Palette &p) {
    if (&p != this) {
        COW_RELEASE(priv);
        priv = COW_REFERENCE(p.priv);
    }
    return *this;
}
auto Palette::copy_group(const Palette &gp, Group dst, Group src) -> void {
    BTK_ASSERT(dst < MaxGroup);
    BTK_ASSERT(src < MaxGroup);
    begin_mut();

    for (uint8_t r = 0; r < MaxRole; r++) {
        priv->brushs[dst][r] = gp.priv->brushs[src][r];
    }
}
auto Palette::brush_at(Group group, Role role) const -> const Brush & {
    BTK_ASSERT(group < MaxGroup);
    BTK_ASSERT(role < MaxRole);
    BTK_ASSERT(priv);
    return priv->brushs[group][role];
}
auto Palette::color_at(Group group, Role role) const -> GLColor {
    return brush_at(group, role).color();
}

auto Palette::set_brush(Group group, Role role, const Brush &b) -> void{
    BTK_ASSERT(group < MaxGroup);
    BTK_ASSERT(role < MaxRole);
    begin_mut();
    priv->brushs[group][role] = b;
}
auto Palette::set_color(Group group, Role role, const GLColor &cl) -> void {
    BTK_ASSERT(group < MaxGroup);
    BTK_ASSERT(role < MaxRole);
    begin_mut();
    priv->brushs[group][role].set_color(cl);
}

auto Palette::to_string() const -> u8string {
    const char *maps[MaxGroup] = {
        "Active",
        "Inactive",
        "Disable"
    };
    const char *rmaps[MaxRole] = {
        "Window",
        "Button",
        "Input",
        "Border",
        "Hightlight",
        "Text",
        "PlaceholderText",
        "HightlightedText",
    };

    u8string result;
    for (uint8_t gp = 0; gp < MaxGroup; gp++) {
        result.append_fmt("[%s]\n", maps[gp]);
        for (uint8_t ro = 0; ro < MaxRole; ro++) {
            auto brush = brush_at(Group(gp), Role(ro));

            switch (brush.type()) {
                case BrushType::Solid : {
                    auto [r, g, b, a] = Color(brush.color());
                    result.append_fmt("%s = #%02X%02X%02X%02X\n", rmaps[ro], int(r), int(g), int(b), int(a));
                    break;
                }
                default : {
                    result.append_fmt("%s = Unsupport formatted brush\n", rmaps[ro]);
                }
            }
        }
    }
    return result;
}

void PaletteBreeze(Palette *palette) {
    // palette->set_color(Palette::Inactive, );
    palette->set_color(Palette::Inactive, Palette::Window, Color(239, 240, 241));
    palette->set_color(Palette::Inactive, Palette::Button, Color(252, 252, 252));
    palette->set_color(Palette::Inactive, Palette::Input, Color(252, 252, 252));
    palette->set_color(Palette::Inactive, Palette::Border, Color(188, 190, 191));
    palette->set_color(Palette::Inactive, Palette::Hightlight, Color(61 , 174, 233));


    // Text
    palette->set_color(Palette::Inactive, Palette::Text, Color::Black);
    palette->set_color(Palette::Inactive, Palette::PlaceholderText, Color::Gray);
    palette->set_color(Palette::Inactive, Palette::HightlightedText, Color::White);

    palette->copy_group(*palette, Palette::Active, Palette::Inactive);
    palette->copy_group(*palette, Palette::Disable, Palette::Inactive);

    palette->set_color(Palette::Active, Palette::Border, Color(61 , 174, 233));
}

// Style
void StyleBreeze(Style *style) {
    style->background = Color(252, 252, 252);
    style->window     = Color(239, 240, 241);
    // style->button     = Color(238, 240, 241);
    style->border     = Color(188, 190, 191);
    style->highlight  = Color(61 , 174, 233);
    style->selection  = Color(61 , 174, 233);

    style->text           = Color(0, 0, 0);
    style->highlight_text = Color(255, 255, 255);

    // Button default size
    style->button_width = 88;
    style->button_height = 32;
    // Button radius
    style->button_radius = 0;

    style->radio_button_circle_pad = 4;
    style->radio_button_circle_inner_pad = 4;
    style->radio_button_circle_r = 8;
    style->radio_button_text_pad = 4;

    // Progress bar
    style->progressbar_width = 100;
    style->progressbar_height = 20;

    // Slider
    style->slider_height = 20;
    style->slider_width  = 20;
    // style->slider_bar_height = 10;
    // style->slider_bar_width  = 10;

    style->scrollbar_width  = 12;
    style->scrollbar_height = 12;

    style->margin = 2.0f;

    // Set font family and size
// #if defined(_WIN32)
#if 0
    // Query current system font
    LOGFONTW lf;
    GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    auto nameu8 = u8string::from(lf.lfFaceName);

    style->font = Font(nameu8, std::abs(lf.lfHeight));
#else
    style->font = Font("", 12);
#endif
    
    // Icon
    style->icon_width = 16;
    style->icon_height = 16;

    // Window default
    style->window_height = 100;
    style->window_width  = 100;

    // Shadow
    style->shadow_radius = 4;
    style->shadow_offset_x = 2;
    style->shadow_offset_y = 2;

    LinearGradient linear;
    linear.add_stop(0, Color::Gray);
    linear.add_stop(1, Color::Transparent);

    style->shadow = linear;

}


BTK_NS_END