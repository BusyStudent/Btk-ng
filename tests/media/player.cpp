#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/dialog.hpp>
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
                
                edit.set_text(dialog.result()[0]);
                player.set_url(dialog.result()[0]);
                player.play();
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
        VideoWidget video;
};

int main() {
    UIContext ctxt;

    Player p;
    p.show();

    VideoWidget v;
    v.show();

    ctxt.run();
}