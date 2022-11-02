#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/dialog.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/context.hpp>
#include <Btk/media.hpp>

using namespace BTK_NAMESPACE;

class Player : public Widget {
    public:
        Player() {
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
            edit.signal_enter_pressed().connect([&]() {
                player.set_url(edit.text());
                player.play();
            });
            player.signal_error().connect([&]() {
                set_window_title("Player Error");
            });
            player.signal_position_changed().connect([&](double pos) {
                set_window_title(u8string::format("Player position %lf : %lf", pos, player.duration()));
                slider.set_value(pos);
            });
            player.signal_duration_changed().connect([&](double dur) {
                slider.set_range(0, dur);
            });

            slider.signal_value_changed().connect([&]() {
                player.set_position(slider.value());
            });

            show();
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
        VideoWidget video;
};

int main() {
    UIContext ctxt;

    Player p;
    p.show();

    VideoWidget v;
    v.show();

    return ctxt.run();
}