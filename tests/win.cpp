#include <Btk/context.hpp>
#include <Btk/widget.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

class Canvas : public Widget {
    public:
        Canvas() : Widget() {
            
            add_timer(100);
            progress.resize(64 , 20);
            progress.set_text_visible(true);

            auto pixbuf = PixBuffer::FromFile("icon.jpeg");
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
        }

        bool paint_event(PaintEvent &e) override {
            auto &gc = this->painter();
            // gc.set_brush(brush);
            gc.set_color(Color::Gray);

            if (x == -1) {
                x = this->width();
            }
            if (y == -1) {
                y = this->height();
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

            return true;
        }
        bool mouse_motion(MotionEvent &e) override {
            x = e.x();
            y = e.y();
            repaint();
            return true;
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

            // repaint();
            return true;
        }

        int x = -1;
        int y = -1;

        int start_x = 0;
        int start_y = 0;

        Brush       brush;
        Brush       linear_brush;
        Brush       radial_brush;

        TextLayout  txt_layout;

        ProgressBar progress = {this};
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
        }

        bool paint_event(PaintEvent &) {
            auto &p = painter();
            p.set_antialias(false);

            if (!text.empty()) {
                auto [w, h] = lay.size();
                p.set_color(Color::Red);
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
        bool textinput_event(TextInputEvent &e) {
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
        bool key_press(KeyEvent &e) {
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

        bool     inside = false;
        int      sel_pos = 0;
};

int main () {
    
    auto device = Win32DriverInfo.create();
    UIContext context(device);

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
    Timer            timer;
    w.set_text("Increment????");

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

    Label lab(&widget," Timer Progress: ");
    lab.move(100 - lab.width(), 120);

    Label lab2(&widget," ProgressBar: ");
    lab2.move(100 - lab.width(), 100);

    Canvas c;
    c.show();
    c.set_window_title("Canvas");

    c.resize(800, 600);

    // Test image view
    ImageView view;
    view.set_image(PixBuffer::FromFile("icon.jpeg"));
    view.set_keep_aspect_ratio(true);
    view.set_window_title("ImageView");
    view.show();

    // Test Text input
    Widget   troot;
    TextEdit tedit(&troot);
    tedit.resize(200, 40);
    tedit.move(10, 10);
    troot.resize(220, 60);
    troot.show();

    tedit.set_placeholder("Please enter text");

    tedit.signal_enter_pressed().connect([&]() {
        troot.set_window_title(tedit.text());
    });

    auto tft = tedit.font();
    tft.set_bold(true);
    tft.set_size(15);
    tedit.set_font(tft);


    // Test lerping
    Point p1(0, 0);
    Point p2(100, 100);

    auto p3 = lerp(p1, p2, 0.5);

    std::cout << p3 << std::endl;

    context.run();
}