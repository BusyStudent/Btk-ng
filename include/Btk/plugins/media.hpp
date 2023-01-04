#pragma once

#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>
#include <Btk/defs.hpp>
#include <Btk/io.hpp>

BTK_NS_BEGIN

class MediaContentImpl;
class MediaPlayerImpl;

class MediaContent;
class MediaPlayer;

// Enum for audio format
enum class AudioSampleFormat : uint8_t {
    Uint8,
    Sint16,
    Sint32,
    Float32
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
        using AudioRoutinue = std::function<void(void *buffer, int byte)>;

        virtual bool open(AudioSampleFormat fmt, int sample_rate, int channals) = 0;
        virtual bool close() = 0;

        virtual void bind(const AudioRoutinue &routinue) = 0;
        virtual void pause(bool paused) = 0;
        virtual bool is_paused() = 0;
};

class VideoWidget : public AbstractVideoSurface, public Widget {
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
class AudioDevice : public AbstractAudioDevice {

};

class MediaStream : public IOStream {
    public:

};

/**
 * @brief Player for media
 * 
 * @warning Known bug : Huge Memory leaks on closed window when the playing is still playing, please stop() when close
 * 
 */
class MediaPlayer : public Object {
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
        void set_media(const MediaContent &content);
        void set_url(u8string_view url);

        void set_position(double ms);

        auto duration() const -> double;
        auto position() const -> double;
        auto error_string() const -> u8string;

        // Signals
        auto signal_error()            -> Signal<void()>       &;
        auto signal_duration_changed() -> Signal<void(double)> &;
        auto signal_position_changed() -> Signal<void(double)> &;
    private:
        MediaPlayerImpl *priv;
};

class MediaContent {
    public:
        MediaContent();
        MediaContent(u8string_view url);
        MediaContent(const MediaContent *ctxts, size_t n);
        ~MediaContent();

        pointer_t query_value(int what);
    private:
        void begin_mut();

        MediaContentImpl *priv;
    friend class MediaPlayer;
};

BTK_NS_END