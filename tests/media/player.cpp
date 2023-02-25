#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/plugins/media.hpp>
#include <Btk/context.hpp>
#include <Btk/event.hpp>
#include <Btk/layout.hpp>

using namespace BTK_NAMESPACE;

class Player : public Widget {
    public:
        Player() {
            edit.set_flat(true);

            auto sub = new BoxLayout;
            lay.attach(this);
            lay.add_layout(sub);
            lay.add_widget(&video, 10);
            lay.add_widget(&slider);

            sub->add_widget(&edit, 1);
            sub->add_widget(&play);
            sub->add_widget(&pause);
            sub->add_widget(&resume);
            sub->add_widget(&stop);

            play.set_text("OpenFile");
            play.signal_clicked().connect([this]() {
                FileDialog dialog;
                dialog.set_option(FileDialog::Open);
                dialog.run();

                auto result = dialog.result();
                if (!result.empty()) {
                    edit.set_text(dialog.result()[0]);
                    player.set_url(dialog.result()[0]);
                    player.play();
                }
            });
            pause.set_text("Pause");
            pause.signal_clicked().connect(&MediaPlayer::pause, &player);
            resume.set_text("Resume");
            resume.signal_clicked().connect(&MediaPlayer::resume, &player);
            stop.set_text("Stop");
            stop.signal_clicked().connect(&MediaPlayer::stop, &player);

            player.set_video_output(&video);
            player.set_audio_output(&audio);
            edit.signal_enter_pressed().connect([&]() {
                player.set_url(edit.text());
                player.play();
            });
            player.signal_error().connect([&]() {
                set_window_title(player.error_string());
            });
            player.signal_position_changed().connect([&](double pos) {
                set_window_title(u8string::format("Player position %lf : %lf", pos, player.duration()));
                slider.set_value(pos);
            });
            player.signal_duration_changed().connect([&](double dur) {
                slider.set_range(0, dur);
            });

            slider.signal_slider_moved().connect([&]() {
                auto value = slider.value();
                printf("Ready seek to %lf\n", value);
                player.set_position(slider.value());
            });

            show();

            auto pal = palette();
            pal.set_color(Palette::Window, Color::Black);

            set_palette(pal);
        }
        bool key_press(KeyEvent &event) override {
            if (event.key() == Key::F11) {
                fullscreen = !fullscreen;
                set_fullscreen(fullscreen);
                return true;
            }
            return false;
        }
        bool mouse_wheel(WheelEvent &event) override {
            // Forward to slider
            return slider.handle(event);
        }
    private:
        BoxLayout lay{TopToBottom};
        MediaPlayer player;
        TextEdit    edit;
        Button      play;
        Button      pause;
        Button      resume;
        Button      stop;
        Slider      slider;
        AudioDevice audio;
        VideoWidget video;

        bool fullscreen = false;
};

int main() {
    UIContext ctxt;

    Player p;
    p.show();

    return ctxt.run();
}