#include <Btk/widgets/textedit.hpp>
#include <Btk/context.hpp>
#include <Btk/media.hpp>

using namespace BTK_NAMESPACE;

int main() {
    UIContext ctxt;
    MediaPlayer player;
    

    Widget root;
    TextEdit edit;
    VideoWidget video;

    BoxLayout lay(TopToBottom);
    lay.attach(&root);
    lay.add_widget(&edit);
    lay.add_widget(&video, 10);

    root.show();

    player.set_video_output(&video);

    edit.signal_enter_pressed().connect([&]() {
        player.set_url(edit.text());
        player.play();
    });

    ctxt.run();
}