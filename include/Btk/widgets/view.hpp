#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

// Label
class BTKAPI Label : public Widget {
    public:
        Label(Widget *parent = nullptr, u8string_view txt = {});
        Label(u8string_view txt) : Label(nullptr, txt) {}
        ~Label();

        void set_text(u8string_view txt);

        bool paint_event(PaintEvent &event) override;
        bool change_event(Event     &event) override;
        Size size_hint() const override; //< Return to text size
    private:
        TextLayout _layout;
        Alignment  _align = Alignment::Left | Alignment::Middle;
};

// Display image
class BTKAPI ImageView : public Widget {
    public:
        ImageView(Widget *parent = nullptr, const PixBuffer *pix = nullptr);
        ImageView(const PixBuffer &pix) : ImageView(nullptr, &pix) {}
        ~ImageView();

        void set_image(const PixBuffer &img);
        void set_keep_aspect_ratio(bool keep);

        bool paint_event(PaintEvent &event) override;
        Size size_hint() const override;
    private:
        Texture  texture;
        PixBuffer pixbuf;

        float radius = 0;
        bool dirty = false;
        bool keep_aspect = false;
};

BTK_NS_END