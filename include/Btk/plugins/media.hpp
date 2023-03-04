#pragma once

#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>
#include <Btk/defs.hpp>
#include <Btk/io.hpp>

BTK_NS_BEGIN

class MediaContentImpl;
class MediaPlayerImpl;
class AudioDeviceImpl;
class VideoFrameImpl;

class MediaContent;
class MediaPlayer;
class AudioDevice;
class VideoFrame;

// Enum for audio format
enum class AudioSampleFormat : uint8_t {
    Uint8,
    Sint16,
    Sint32,
    Float32
};
enum class AudioChannelLayout : uint8_t {

};
enum class MediaStatus       : uint8_t {
    NoMedia,
    LoadingMedia,
    LoadedMedia,
    StalledMedia,
    BufferingMedia,
    BufferedMedia,
    EndOfMedia
};

/**
 * @brief Audio format 
 * 
 */
class AudioFormat {
    public:
        AudioSampleFormat sample_fmt;
        int               sample_rate;
        int               channels;
};

// Interface, needed for polymorphism, mixed in with other classes
class AbstractVideoSurface {
    public:
        virtual bool begin(PixFormat *req) = 0;
        virtual bool write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) = 0;
        virtual bool end() = 0;
};
class AbstractAudioDevice {
    public:
        using Routinue = std::function<void(void *buffer, uint32_t bytes)>;

        virtual bool open(AudioSampleFormat fmt, int sample_rate, int channels) = 0;
        virtual bool close() = 0;

        virtual void bind(const Routinue &routinue) = 0;
        virtual void pause(bool paused) = 0;
        virtual bool is_paused() = 0;

        virtual void  set_volume(float volume) = 0;
        virtual float volume() = 0;
};

class BTKAPI VideoWidget : public AbstractVideoSurface, public Widget {
    public:
        VideoWidget(Widget *parent = nullptr);
        ~VideoWidget();

        bool paint_event(PaintEvent &) override;
    private:
        bool begin(PixFormat *req) override;
        bool write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) override;
        bool end() override;

        bool    playing = false;
        Color   background = Color::Black;
        Size    native_resolution = {0, 0};
        Texture texture;
};

// Bultin audio device implementation
class BTKAPI AudioDevice : public AbstractAudioDevice, public Any {
    public:
        AudioDevice();
        AudioDevice(const AudioDevice &) = delete;
        ~AudioDevice();

        bool open(AudioSampleFormat fmt, int sample_rate, int channels) override;
        bool close() override;

        void bind(const Routinue &routinue) override;
        void pause(bool paused) override;
        bool is_paused() override;

        void  set_volume(float volume) override;
        float volume() override;
    private:
        AudioDeviceImpl *priv;
};

class BTKAPI MediaStream : public IOStream {
    public:

};

/**
 * @brief Player for media
 * 
 * @warning Known bug : Huge Memory leaks on closed window when the playing is still playing, please stop() when close
 * 
 */
class BTKAPI MediaPlayer : public Object {
    public:
        enum State {
            Playing,
            Paused,
            Stopped,
        };

        MediaPlayer();
        ~MediaPlayer();

        void resume();
        void pause();
        void play();
        void stop();

        void set_video_output(AbstractVideoSurface *surf);
        void set_audio_output(AbstractAudioDevice *audio);
        void set_option(u8string_view key, u8string_view value);
        void set_url(u8string_view url);

        void set_position(double second);

        auto state() const -> State;
        auto seekable() const -> bool;
        auto buffered() const -> double;
        auto duration() const -> double;
        auto position() const -> double;
        auto error_string() const -> u8string;
        auto media_status() const -> MediaStatus;

        // Signals
        auto signal_error()            -> Signal<void()>       &;
        auto signal_duration_changed() -> Signal<void(double)> &;
        auto signal_position_changed() -> Signal<void(double)> &;
        auto signal_buffer_status_changed() -> Signal<void(float)>       &;
        auto signal_media_status_changed()  -> Signal<void(MediaStatus)> &;
        auto signal_state_changed()         -> Signal<void(State)>       &;
    private:
        MediaPlayerImpl *priv;
};

BTK_NS_END