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
void MediaPlayer::set_option(u8string_view key, u8string_view value) {
    priv->set_option(key, value);
}

auto MediaPlayer::signal_media_status_changed() -> Signal<void(MediaStatus)> & {
    return priv->signal_media_status_changed();    
}
auto MediaPlayer::signal_duration_changed() -> Signal<void(double)> & {
    return priv->signal_duration_changed();
}
auto MediaPlayer::signal_position_changed() -> Signal<void(double)> & {
    return priv->signal_position_changed();
}
auto MediaPlayer::signal_state_changed() -> Signal<void(State)> & {
    return priv->signal_state_changed();
}
auto MediaPlayer::signal_error() -> Signal<void()> & {
    return priv->signal_error();
}

bool MediaPlayer::seekable() const {
    return priv->seekable();
}
double MediaPlayer::buffered() const {
    return priv->buffered();
}
double MediaPlayer::duration() const {
    return priv->duration();
}
double MediaPlayer::position() const {
    return priv->position();
}
u8string MediaPlayer::error_string() const {
    char buffer[AV_ERROR_MAX_STRING_SIZE];
    int errcode = priv->error_code();
    return av_make_error_string(buffer, sizeof(buffer), errcode);
}


BTK_NS_END