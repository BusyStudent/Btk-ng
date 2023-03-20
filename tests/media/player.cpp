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
            pause.signal_clicked().connect(&Player::on_pause_clicked, this);
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
                set_window_title(u8string::format("Player position %lf : %lf volume %f", pos, player.duration(), audio.volume()));
                slider.set_value(pos);
            });
            player.signal_buffer_status_changed().connect([&](int percent) {
                set_window_title(u8string::format("Buffer %d%%", percent));
            });
            player.signal_duration_changed().connect([&](double dur) {
                slider.set_range(0, dur);
            });
            player.signal_state_changed().connect(&Player::on_state_changed, this);

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
            printf("Repeat %d\n", int(event.repeat()));

            if (event.key() == Key::F11) {
                fullscreen = !fullscreen;
                set_fullscreen(fullscreen);

                edit.set_visible(!fullscreen);
                play.set_visible(!fullscreen);
                pause.set_visible(!fullscreen);
                stop.set_visible(!fullscreen);
                return true;
            }
            else if (event.key() == Key::Space) {
                // Like 
                on_pause_clicked();
                return true;
            }
            else if (event.key() == Key::Right) {
                // Add position
                player.set_position(min(player.position() + 10.0, player.duration()));
                return true;
            }
            else if (event.key() == Key::Left) {
                // Add position
                player.set_position(max(player.position() - 10.0, 0.0));
                return true;
            }
            else if (event.key() == Key::Up) {
                // Add volume
                audio.set_volume(min(audio.volume() + 0.1f, 1.0f));
                return true;
            }
            else if (event.key() == Key::Down) {
                // Add volume
                audio.set_volume(max(audio.volume() - 0.1f, 0.0f));
                return true;
            }
            return false;
        }
        bool mouse_wheel(WheelEvent &event) override {
            // Forward to slider
            return slider.handle(event);
        }
    private:
        void on_pause_clicked() {
            if (player.state() == MediaPlayer::Paused) {
                player.resume();
            }
            else {
                player.pause();
            }
        }
        void on_state_changed(MediaPlayer::State state) {
            if (state == MediaPlayer::Paused) {
                pause.set_text("Resume");
            }
            else {
                pause.set_text("Pause");
            }
        }

        BoxLayout lay{TopToBottom};
        MediaPlayer player;
        TextEdit    edit;
        Button      play;
        Button      pause;
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