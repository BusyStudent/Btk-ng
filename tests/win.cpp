#include <Btk/widgets/dialog.hpp>
#include <Btk/widgets/menu.hpp>
#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <Btk/comctl.hpp>
#include <Btk/event.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

PixBuffer test_image();

class Canvas : public Widget {
    public:
        Canvas() : Widget() {
            // set_attribute(WidgetAttrs::Debug, true);
            
            add_timer(100);
            progress.resize(64 , 20);
            progress.set_text_visible(true);

            auto pixbuf = test_image();
            brush.set_image(pixbuf);

            // Configure linear gradient
            LinearGradient g;
            g.add_stop(0.0, Color::Red);
            g.add_stop(1.0, Color::Green);

            linear_brush.set_gradient(g);

            // Configure radial gradient
            RadialGradient g2;
            g2.add_stop(0.0, Color::Red);
            g2.add_stop(1.0, Color::Blue);

            radial_brush.set_gradient(g2);

            // Configure text layout
            txt_layout.set_font(font());
            txt_layout.set_text("Hello Canvas !");

            // Try query the text size 
            auto [w, h] = txt_layout.size();
            printf("%d %d\n", w, h);

            // Set its icon
            set_window_icon(pixbuf);

            // Make the path
            path.open();
            path.move_to(500, 500);
            path.quad_to(500, 100, 100, 100);
            path.bezier_to(700, 800, 0, 0, 122, 236);
            path.close_path();
            path.close();

            arc_path.open();
            arc_path.move_to(0, 0);
            arc_path.arc_to(50, 50, 0, 100, 40);
            arc_path.close();

            dash_pen.set_dash_style(DashStyle::DashDotDot);
            dash_pen.set_line_cap(LineCap::Round);

            scroll.signal_value_changed().connect(&Canvas::repaint, this);
        }

        bool paint_event(PaintEvent &e) override {
            auto &gc = this->painter();
            // gc.set_brush(brush);
            ytranslate = height() * (scroll.value() / 100.0f);

            gc.push_transform();
            gc.translate(0, -ytranslate);
            gc.set_color(Color::Gray);

            if (x == -1) {
                x = this->width();
            }
            if (y == -1) {
                y = this->height() + ytranslate;
            }

            FPoint points [] = {
                FPoint(start_x, start_y),
                FPoint(x, y)
            };

            // printf("Canvas::paint_event: %d %d\n", this->width(), this->height());
            gc.set_antialias(true);
            gc.draw_lines(points,2);

            // Try make a rect at center of screen
            gc.set_brush(brush);
            float center_x = this->width() / 2;
            float center_y = this->height() / 2;

            float width = 100;
            width = max(width, x - center_x);

            float height = 100;
            height = max(height, y - center_y);

            gc.fill_rounded_rect(center_x, center_y, width, height, 10);

            // Make a linear gradient
            gc.set_brush(linear_brush);
            gc.fill_rect(100, 0, 100, 100);

            gc.set_brush(radial_brush);
            gc.fill_circle(150, 150, 50);

            gc.set_antialias(true);
            gc.set_color(Color::Cyan);
            gc.draw_rounded_rect(200, 200, 100, 100, 10);
            gc.set_antialias(true);

            gc.set_stroke_width(stroke_width);
            gc.set_brush(linear_brush);
            gc.set_pen(dash_pen);
            gc.draw_path(path);
            gc.set_stroke_width(1);

            gc.push_transform();
            gc.translate(x, y);
            gc.scale(2, 2);
            gc.draw_path(arc_path);
            gc.pop_transform();

            // Reset pen
            gc.set_pen(nullptr);
            gc.pop_transform();

            return true;
        }
        bool mouse_motion(MotionEvent &e) override {
            x = e.x();
            y = e.y();
            repaint();
            return true;
        }
        bool mouse_wheel(WheelEvent &e) override {
            
            if (e.y() > 0) {
                stroke_width += 1;
            }
            else {
                stroke_width -= 1;
                stroke_width = max(1.0f, stroke_width);
            }

            repaint_now();

            return true;
        }
        bool mouse_release(MouseEvent &e) override {
            if (e.button() == MouseButton::Right) {
                Point p = mouse_position();

                auto wi = new PopupWidget();
                wi->resize(50, 50);
                wi->move(p.x, p.y);
                wi->popup(this);
                wi->set_attribute(WidgetAttrs::DeleteOnClose, true);
                return true;
            }
            return false;
        }
        bool timer_event(TimerEvent &) override {
            // x = rand() % this->width();
            // y = rand() % this->height();

            // start_x = rand() % this->width();
            // start_y = rand() % this->height();

            if (progress.value() >= 100) {
                progress.set_value(0);
            }
            else {
                progress.set_value(progress.value() + 1);
            }
            // FMatrix mat;
            // mat.translate(progress.value(), 0);
            // path.set_transform(mat);
            dash_pen.set_dash_offset(progress.value());

            // repaint();
            return true;
        }
        bool key_press(KeyEvent &event) override {
            if (event.key() == Key::F11) {
                fullscreen = !fullscreen;
                set_fullscreen(fullscreen);
                return true;
            }
            return false;
        }
        bool resize_event(ResizeEvent &event) override {
            auto bar_w = scroll.size_hint().w;
            scroll.move(event.width() - bar_w, 0);
            scroll.resize(bar_w, event.height());
            return true;
        }

        int x = -1;
        int y = -1;

        int start_x = 0;
        int start_y = 0;

        float       stroke_width = 2.0f;
        float       ytranslate   = 0.0f;
        bool        fullscreen = false;

        Brush       brush;
        Brush       linear_brush;
        Brush       radial_brush;
        Pen         dash_pen;

        TextLayout  txt_layout;
        PainterPath arc_path;
        PainterPath path;

        ProgressBar progress = {this};
        ScrollBar   scroll   = {this, Vertical};
};
class Editer : public Widget {
    public:
        Editer() : Widget() {
            set_window_title("Editor");
            start_textinput();

            auto f = font();
            f.set_size(20);
            f.set_bold(true);

            lay.set_font(f);

            LinearGradient g;
            g.add_stop(0.0, Color::Red);
            g.add_stop(1.0, Color::Cyan);
            brush.set_gradient(g);
        }

        bool paint_event(PaintEvent &) override {
            auto &p = painter();
            p.set_antialias(false);

            if (!text.empty()) {
                // Drop shadow below the background box 
                p.set_brush(style()->shadow);
                FRect nr = {
                    rect.x + style()->shadow_offset_x,
                    rect.y + rect.h,
                    rect.w,
                    4.0f
                };
                p.fill_rect(nr);

                // Fill background
                p.set_color(Color::White);
                p.fill_rect(rect);

                auto [w, h] = lay.size();
                p.set_brush(brush);
                p.set_font(font());
                p.set_text_align(Alignment::Left + Alignment::Top);
                p.draw_text(lay, rect.x, rect.y);

                // Draw a box
                p.set_color(bounds);
                p.draw_rect(rect);

                // Draw a line
                p.set_color(Color::Green);
                if (sel_pos == 0) {
                    p.draw_line(rect.x + 1, rect.y, rect.x + 1, rect.y + h);
                }

                if (inside) {
                    printf("Draw box");

                    auto r = box;
                    r.x += rect.x;
                    r.y += rect.y;
                    p.set_color(Color::Blue);
                    p.draw_rect(r);
                }
            }
            return true;
        }
        bool textinput_event(TextInputEvent &e) override {
            text.append(e.text());
            lay.set_text(text);

            auto [w, h] = lay.size();
            rect.w = w;
            rect.h = h;
            auto r = rect.cast<int>();
            set_textinput_rect(r);


            repaint();
            return true;
        }
        bool key_press(KeyEvent &e) override {
            if (e.key() == Key::Backspace) {
                text.pop_back();
                lay.set_text(text);
                auto [w, h] = lay.size();
                rect.w = w;
                rect.h = h;
                repaint();
            }
            else if (e.key() == Key::Up) {
                rect.y -= 10;
                set_textinput_rect(rect.cast<int>());
                repaint();
            }
            else if (e.key() == Key::Down) {
                rect.y += 10;
                set_textinput_rect(rect.cast<int>());
                repaint();
            }
            else if (e.key() == Key::Left) {
                rect.x -= 10;
                set_textinput_rect(rect.cast<int>());
                repaint();
            }
            else if (e.key() == Key::Right) {
                rect.x += 10;
                set_textinput_rect(rect.cast<int>());
                repaint();
            }
            return true;
        }
        bool mouse_press(MouseEvent &event) override {
            if (text.empty()) {
                return true;
            }
            auto pos = event.position().cast<float>() - rect.position();

            TextHitResult result;
            lay.hit_test(pos.x, pos.y, &result);

            if (result.inside) {
                bounds = Color::Blue;
            }
            else {
                printf("Outside\n");
                bounds = Color::Black;
            }
            if (result.trailing) {
                printf("Trailing\n");
            }
            else {
                printf("Leading\n");
            }
            printf("%d %d\n", int(result.text), int(result.length));
            std::cout << result.box << std::endl;

            inside = result.inside;
            box    = result.box;

            repaint();
            return true;
        }
    public:
        TextLayout lay;
        u8string text;
        FRect    rect = {0.0f, 0.0f, 0.0f, 0.0f};
        FRect    box;
        float    step = 10.0f;
        GLColor  bounds = Color::Black;
        Brush    brush;

        bool     inside = false;
        int      sel_pos = 0;
};

PixBuffer test_image() {
    auto buf = PixBuffer::FromFile("icon.jpeg");
    if (buf.empty()) {
        buf = PixBuffer(PixFormat::RGBA32, 500, 500);

        auto painter = Painter::FromPixBuffer(buf);
        painter.begin();
        painter.set_color(Color::Red);
        painter.fill_rect(0, 0, 500, 500);
        painter.end();
    }
    return buf;
}

int main () {
    
    UIContext context;

    // PixBuffer pixbuf(100, 100);
    // auto painter = Painter::FromPixBuffer(pixbuf);
    // painter.begin();
    // painter.draw_line(0, 0, 100, 100);
    // painter.end();
    
    // Make a widget and put a button on it
    Widget widget;
    Button w(&widget);
    Button v(&widget);
    Button u(&widget);
    ProgressBar p(&widget);
    ProgressBar q(&widget);
    Slider      s(&widget);
    RadioButton rb(&widget);
    Label       label(&widget);

    Timer            timer;
    w.set_text("Increment😀");

    widget.show();
    widget.resize(640, 480);

    v.move(100,0 );
    v.set_text("Text Visible");


    p.set_value(50);
    p.move(100, 100);
    p.resize(200, 20);

    q.set_value(0);
    q.move(100, 120);
    q.resize(200, 20);
    
    // Connect signal
    w.signal_clicked().connect([&]() {
        std::cout << "Button clicked" << std::endl;
        if (p.value() == 99) {
            p.set_value(0);
        }
        else{
            p.set_value(p.value() + 1);
        }
    });
    v.signal_clicked().connect([&, vis = true]() mutable {
        std::cout << "Button clicked" << std::endl;
        p.set_text_visible(vis);
        q.set_text_visible(vis);
        vis = !vis;
    });

    u.move(100, 140);
    u.set_text("Start timer");
    u.signal_clicked().connect([&, started = false]() mutable {
        if (started) {
            timer.stop();
            started = false;
            u.set_text("Start timer");
        }
        else {
            timer.start();
            started = true;
            u.set_text("Stop timer");
        }
    });

    timer.signal_timeout().connect([&]() {
        if (q.value() == 100) {
            widget.close();
            timer.stop();
        }
        else{
            q.set_value(q.value() + 1);
        }
    });

    timer.set_interval(10);
    timer.set_repeat(true);

    Label lab(&widget, " Timer Progress: ");
    lab.move(100 - lab.width(), 120);

    Label lab2(&widget, " ProgressBar: ");
    lab2.move(100 - lab.width(), 100);

    auto lfont = label.font();
    lfont.set_size(20);
    lfont.set_bold(true);
    label.set_font(lfont);
    label.set_text("All useable widgets");
    label.move(100, 70);
    label.resize(label.adjust_size());

    // Move The Slider
    s.move(100, 170);
    s.resize(200, 20);
    s.signal_value_changed().connect([&]() {
        p.set_value(s.value());
    });

    rb.move(100, 190);
    rb.set_text("This is a radio button");
    rb.resize(200, 32);

    Canvas c;
    c.show();
    c.set_window_title("Canvas");

    c.resize(800, 600);

    // Test image view
    ImageView view;
    view.set_image(test_image());
    view.set_keep_aspect_ratio(true);
    view.set_window_title("ImageView");
    view.show();
    view.add_event_filter([](Object *w, Event &event, void *data) {
        if (event.type() == Event::KeyRelease) {
            if (event.as<KeyEvent>().key() == Key::Return) {
                FileDialog dialog;
                dialog.set_option(FileDialog::Open);
                dialog.run();
                auto result = dialog.result();
                if (!result.empty()) {
                    static_cast<ImageView*>(w)->set_image(Image::FromFile(result[0]));
                }
            }
        }
        return false;
    }, nullptr);

    // Test Text input
    TextEdit tedit(&widget);
    tedit.resize(200, 32);
    tedit.move(200, 0);

    // Test Hit test
    Editer   editer;
    editer.set_window_title("Test TextLayout hit test");
    editer.show();

    tedit.set_placeholder("Please enter text");

    tedit.signal_enter_pressed().connect([&]() {
        widget.set_window_title(tedit.text());
    });

    auto tft = tedit.font();
    tft.set_bold(true);
    tft.set_size(15);
    tedit.set_font(tft);

    // Test layout
    Widget lay_root;
    Button lay_btn1("Button 1");
    Button lay_btn2("Button 2");
    Button lay_btn3("Button 3");
    BoxLayout layout(LeftToRight);

    layout.attach(&lay_root);
    layout.add_widget(&lay_btn1, 1, Alignment::Middle | Alignment::Center);
    layout.add_widget(&lay_btn2, 2, Alignment::Middle | Alignment::Center);
    layout.add_spacing(50);
    layout.add_widget(&lay_btn3, 1, Alignment::Middle | Alignment::Center);

    auto sublay = new BoxLayout(TopToBottom);
    layout.add_layout(sublay);
    sublay->add_widget(new Button("SubButton 1"));
    sublay->add_widget(new Button("SubButton 2"));
    sublay->add_widget(new Button("SubButton 3"));
    sublay->add_stretch(1);

    layout.add_stretch(1);

    lay_root.show();


    context.run();
}