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

// Interface, needed for polymorphism, mixed in with other classes
class AbstractVideoSurface {
    public:
        virtual bool begin(PixFormat *req) = 0;
        virtual bool write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) = 0;
        virtual bool end() = 0;
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

        Color   background = Color::Black;
        Size    native_resolution = {0, 0};
        Texture texture;
};

class MediaStream : public IOStream {
    public:

};

class MediaPlayer : public Object {
    public:
        enum State {
            Playing,
            Paused,
            Stopped
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

        // Signals
        auto signal_error() -> Signal<void()> &;
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