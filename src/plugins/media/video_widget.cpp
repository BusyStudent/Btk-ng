#include "build.hpp"

#include <Btk/plugins/media.hpp>

BTK_NS_BEGIN

VideoWidget::VideoWidget(Widget *p) : Widget(p) {

}
VideoWidget::~VideoWidget() {

}
bool VideoWidget::paint_event(PaintEvent &) {
    auto &p = painter();
    p.set_color(background);
    p.fill_rect(rect());

    if (!texture.empty()) {
        // Keep aspect ratio
        FRect dst;
        FSize size = rect().size();
        FSize img_size = texture.size();

        if (img_size.w * size.h > img_size.h * size.w) {
            dst.w = size.w;
            dst.h = img_size.h * size.w / img_size.w;
        }
        else {
            dst.h = size.h;
            dst.w = img_size.w * size.h / img_size.h;
        }
        dst.x = rect().x + (size.w - dst.w) / 2;
        dst.y = rect().y + (size.h - dst.h) / 2;
        p.draw_image(texture, &dst, nullptr);
    }
    return true;
}
bool VideoWidget::begin(PixFormat *fmt) {
    *fmt = PixFormat::RGBA32;
    playing = true;
    return true;
}
bool VideoWidget::end() {
    if (!ui_thread()) {
        defer_call(&VideoWidget::end, this);
        return true;
    }

    playing = false;
    texture.clear();
    repaint();
    return true;
}
bool VideoWidget::write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) {
    if (!ui_thread()) {
        defer_call(&VideoWidget::write, this, fmt, data, pitch, w, h);
        return true;
    }

    assert(fmt == PixFormat::RGBA32);

    if (!playing) {
        return false;
    }

    native_resolution = {w, h};
    if (texture.empty() || texture.size() != native_resolution) {
        texture = painter().create_texture(fmt, w, h);
        // texture.set_interpolation_mode(InterpolationMode::Nearest);
    }
    texture.update(nullptr, data, pitch);
    repaint();
    return true;
}

BTK_NS_END