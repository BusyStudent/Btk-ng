#include "build.hpp"
#include "media_priv.hpp"

BTK_NS_BEGIN

class MediaPlayerImpl : public FFmpeg::Demuxer { };

// MediaPlayer
MediaPlayer::MediaPlayer() {
    priv = new MediaPlayerImpl;
}
MediaPlayer::~MediaPlayer() {
    delete priv;
}

void MediaPlayer::play() {
    priv->play();
}
void MediaPlayer::stop() {
    priv->stop();
}
void MediaPlayer::resume() {
    priv->pause(false);
}
void MediaPlayer::pause() {
    priv->pause(true);
}
void MediaPlayer::set_url(u8string_view url) {
    priv->set_url(url);
}
void MediaPlayer::set_video_output(AbstractVideoSurface *v) {
    priv->set_video_output(v);
}
void MediaPlayer::set_audio_output(AbstractAudioDevice *a) {
    priv->set_audio_output(a);
}
void MediaPlayer::set_position(double pos) {
    priv->set_position(pos);
}

auto MediaPlayer::signal_duration_changed() -> Signal<void(double)> & {
    return priv->signal_duration_changed();
}
auto MediaPlayer::signal_position_changed() -> Signal<void(double)> & {
    return priv->signal_position_changed();
}
auto MediaPlayer::signal_error() -> Signal<void()> & {
    return priv->signal_error();
}

double MediaPlayer::duration() const {
    return priv->duration();
}
double MediaPlayer::position() const {
    return priv->position();
}
u8string MediaPlayer::error_string() const {
    return "Not Impl";
}


BTK_NS_END