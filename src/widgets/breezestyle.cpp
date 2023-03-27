#include "build.hpp"
#include <Btk/style.hpp>


BTK_PRIV_BEGIN

class BreezeStyle final : public Style {
    public:
        BreezeStyle();

        // Draw
        void draw_control(Control ctl, Widget *wi, Painter &p) override; 
};

extern "C" void __BtkWidgets_STYLE_Init() {
    RegisterStyle<BreezeStyle>("Breeze");
}

BreezeStyle::BreezeStyle() {
    // background = Color(252, 252, 252);
    // window     = Color(239, 240, 241);
    // // button     = Color(238, 240, 241);
    // border     = Color(188, 190, 191);
    // highlight  = Color(61 , 174, 233);
    // selection  = Color(61 , 174, 233);

    // text           = Color(0, 0, 0);
    // highlight_text = Color(255, 255, 255);

    // Button default size
    button_width = 88;
    button_height = 32;
    // Button radius
    button_radius = 0;
    button_icon_spacing = 4;

    radio_button_circle_pad = 4;
    radio_button_circle_inner_pad = 4;
    radio_button_circle_r = 8;
    radio_button_text_pad = 4;

    // Progress bar
    progressbar_width = 100;
    progressbar_height = 20;

    // Slider
    slider_height = 20;
    slider_width  = 20;
    // slider_bar_height = 10;
    // slider_bar_width  = 10;

    scrollbar_width  = 12;
    scrollbar_height = 12;

    margin = 2.0f;

    // Set font family and size
// #if defined(_WIN32)
#if 0
    // Query current system font
    LOGFONTW lf;
    GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    auto nameu8 = u8string::from(lf.lfFaceName);

    font = Font(nameu8, std::abs(lf.lfHeight));
#else
    const char *sysfont = ::getenv("BTK_SYSFONT");
    if (!sysfont) {
        sysfont = "Noto Sans Regular";
    }
    font = Font(sysfont, 12);
#endif
    
    // Icon
    icon_width = 20;
    icon_height = 20;

    // Window default
    window_height = 100;
    window_width  = 100;

    // Shadow
    shadow_radius = 4;
    shadow_offset_x = 2;
    shadow_offset_y = 2;

    LinearGradient linear;
    linear.add_stop(0, Color::Gray);
    linear.add_stop(1, Color::Transparent);
    linear.set_start_point(0.5f, 0.0f);
    linear.set_end_point(0.5f, 1.0f);

    shadow = linear;
}
void BreezeStyle::draw_control(Control ctl, Widget *wi, Painter &p) {

}


BTK_PRIV_END