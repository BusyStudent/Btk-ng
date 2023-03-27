#include "build.hpp"
#include "common/utils.hpp"
#include "common/pod_container.hpp"

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
    group = p.group;
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

Palette PaletteBreeze() {
    Palette palette;
    // Make brush for button actived / inactived
    LinearGradient gradient;
    gradient.set_start_point(0.5f, 0.0f);
    gradient.set_end_point(0.5f  , 1.0f);
    gradient.add_stop(0.0f, Color(241, 242, 243));
    gradient.add_stop(1.0f, Color(232, 233, 234));
    palette.set_brush(Palette::Button, gradient); //< Set for all groups
    palette.set_brush(Palette::ButtonHovered, gradient);
    // Make for pressed
    gradient.clear();
    gradient.add_stop(0.0f, Color(150, 210, 238));
    gradient.add_stop(1.0f, Color(134, 188, 212));
    palette.set_brush(Palette::ButtonPressed, gradient);
    palette.set_brush(Palette::Active, Palette::ButtonHovered, gradient); //< As same as active
    // Make for nohovered active button
    palette.set_brush(Palette::Active, Palette::Button, Color(61 , 174, 233)); 
    
    // Color(252, 252, 252);

    palette.set_color(Palette::Window, Color(239, 240, 241));
    palette.set_color(Palette::Input, Color(252, 252, 252));
    palette.set_color(Palette::Border, Color(179, 181, 182));
    palette.set_color(Palette::Hightlight, Color(61 , 174, 233));


    // Text
    palette.set_color(Palette::Text, Color::Black);
    palette.set_color(Palette::PlaceholderText, Color::Gray);
    palette.set_color(Palette::HightlightedText, Color::White);

    palette.set_color(Palette::Shadow, Color::Gray);
    palette.set_color(Palette::Light, Color(61 , 174, 233, 255 / 2.5));

    // palette.set_color(Palette::Active, Palette::Border, Color(61 , 174, 233));
    // Active group
    palette.set_brush(Palette::Active, Palette::Border, Color(166, 216, 243));

    return palette;
}

// Style
static CompressedDict<Ref<Style> (*)()> &
GetStyleList() {
    static CompressedDict<Ref<Style> (*)()> list;
    return list;
}

void       RegisterStyle(u8string_view name, Ref<Style> (*fn)()) {
    auto &list = GetStyleList();
    list.push_back(name, fn);
}
Ref<Style> CreateStyle(u8string_view name) {
    auto &list = GetStyleList();
    if (list.empty()) {
        return nullptr;
    }
    if (name.empty()) {
        return list.back().second();
    }
    for (auto [n, create] : list) {
        if (name == n) {
            return create();
        }
    }
    return nullptr;
}


BTK_NS_END