#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/media.hpp>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <queue>

// Import FFmpeg
extern "C" {
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/avutil.h>
}

BTK_NS_BEGIN

namespace {
    template <typename T, auto Alloc, auto Free>
    class FFTraits {
        public:
            void operator()(T *v) {
                Free(v);
            }
            static T *_Alloc() {
                return Alloc();
            }
    };
    template <typename T, typename Traits>
    class FFPtr : public std::unique_ptr<T, Traits> {
        public:
            using std::unique_ptr<T, Traits>::unique_ptr;

            template <typename ...Args>
            static FFPtr Alloc(Args && ...args) {
                return FFPtr(Traits::_Alloc(std::forward<Args>(args)...));
            }
    };
    template <typename T>
    T   *FFUnused() {
        assert(false);
        abort();
    }
    template <typename T, auto Free>
    void FFPtr2Warp(T *ptr) {
        Free(&ptr);
    }

    template <typename T, auto Alloc, auto Free>
    using FFWrap = FFPtr<T, FFTraits<T, Alloc, Free>>;

    using AVFormatContextPtr = FFWrap<AVFormatContext, FFUnused<AVFormatContext>, FFPtr2Warp<AVFormatContext, avformat_close_input>>;
    using AVCodecContextPtr  = FFWrap<AVCodecContext,  FFUnused<AVCodecContext>,  avcodec_close>;
    using AVPacketPtr = FFWrap<AVPacket, av_packet_alloc, FFPtr2Warp<AVPacket, av_packet_free>>;
    using AVFramePtr  = FFWrap<AVFrame,  av_frame_alloc , FFPtr2Warp<AVFrame , av_frame_free>>;
    using SwsContextPtr = FFWrap<SwsContext, sws_getContext, sws_freeContext>;

    static std::once_flag device_once;

    static u8string format_fferror(int errc) {
        u8string ret;
        ret.resize(AV_ERROR_MAX_STRING_SIZE);

        av_make_error_string(ret.data(), AV_ERROR_MAX_STRING_SIZE, errc);
        return ret;
    }
}

// Impl for media context
class MediaContentImpl : public Refable<MediaContentImpl> {
    public:
        AVFormatContextPtr                ctxt;
};
class MediaPlaylist    : public MediaContentImpl {
    public:
        std::vector<Ref<MediaContent>> subs;
};

// Impl for MediaPlayer
class MediaPlayerImpl  : public Object {
    public:
        using State = MediaPlayer::State;

        MediaPlayerImpl();
        ~MediaPlayerImpl();

        void play();
        void stop();
        void pause();
        void resume();

        AVFormatContextPtr container   = {};
        AVCodecContextPtr  video_ctxt  = {};
        AVFramePtr         video_frame = {}; //< For raw decoded data
        AVFramePtr         video_frame2= {}; //< For format converted data
        const AVCodec     *video_codec = {};
        SwsContextPtr      video_cvt   = {};
        AVCodecContextPtr  audio_ctxt  = {};
        const AVCodec     *audio_codec = {};
        AVFramePtr         audio_frame = AVFramePtr::Alloc(); //< For raw decoded audio data
        AVPacketPtr        packet      = AVPacketPtr::Alloc();

        AbstractVideoSurface   *video_output = nullptr;
        u8string                video_url    = {};

        std::mutex              mutx;
        std::thread             thrd;
        std::condition_variable cond;

        std::atomic<State> state   = State::Stopped;
        std::atomic<bool>  running = true;

        int   video_stream = -1;
        int   audio_stream = -1;

        Signal<void()> signal_played;
        Signal<void()> signal_paused;
        Signal<void()> signal_stopped;
        Signal<void()> signal_error;

        // Video frames queue

        bool timer_event(TimerEvent &) override;
        void codec_thread();
        void proc_video();
        void proc_audio();
};


// Begin video player
MediaPlayerImpl::MediaPlayerImpl() {
    std::call_once(device_once, avdevice_register_all);

    thrd = std::thread(&MediaPlayerImpl::codec_thread, this);
}
MediaPlayerImpl::~MediaPlayerImpl() {
    state = State::Stopped;
    running = false;
    cond.notify_one();
    thrd.join();
}

void MediaPlayerImpl::codec_thread() {
    while (running) {
        if (state != State::Playing) {
            // Wait for sth changed
            std::unique_lock lk(mutx);
            cond.wait(lk, [this]() {
                return !running || state == State::Playing;
            });
        }
        if (!container) {
            continue;
        }
        while (av_read_frame(container.get(), packet.get()) >= 0 && state == State::Playing) {
            // Process
            double pts = packet->pts * av_q2d(packet->time_base);
            if (packet->stream_index == video_stream) {
                proc_video();
            }
            else if (packet->stream_index == audio_stream) {

            }

            // Done
            av_packet_unref(packet.get());
        }

        // EOF
        if (state == State::Playing) {
            state = State::Stopped;
            defer_call(&Signal<void()>::emit, &signal_error);
        }
    }
}
void MediaPlayerImpl::proc_video() {
    if (!video_output) {
        // No need to proc, drop
        return;
    }

    int ret = avcodec_send_packet(video_ctxt.get(), packet.get());
    if (ret != 0) {
        // Error
        return;
    }
    ret = avcodec_receive_frame(video_ctxt.get(), video_frame.get());
    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
        // We need more packet
        return;
    }

    // Process frame
    ret = sws_scale(
        video_cvt.get(),
        video_frame->data,
        video_frame->linesize,
        0,
        video_ctxt->height,
        video_frame2->data,
        video_frame2->linesize
    );
    // sws_scale_frame(video_cvt.get(), video_frame2.get(), video_frame.get());

    // Push to surface
    defer_call([this]() {
        video_output->write(PixFormat::RGBA32, video_frame2->data[0], video_frame2->linesize[0], video_ctxt->width, video_ctxt->height);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
}
void MediaPlayerImpl::play() {
    if (video_url.empty()) {
        return;
    }
    AVFormatContext *ps = nullptr;
    int ret = avformat_open_input(
        &ps,
        video_url.c_str(),
        nullptr,
        nullptr
    );
    if (ret != 0) {
        // Error
        signal_error.emit();
        return;
    }
    container.reset(ps);

    ret = avformat_find_stream_info(container.get(), nullptr);
    if (ret != 0) {
        // Error
        signal_error.emit();
        return;
    }

    // Try find stream
    video_stream = av_find_best_stream(
        container.get(),
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        nullptr,
        0
    );
    if (video_stream < 0) {
        // Error
        video_stream = -1;
    }
    audio_stream = av_find_best_stream(
        container.get(),
        AVMEDIA_TYPE_AUDIO,
        -1,
        video_stream,
        nullptr,
        0
    );
    if (audio_stream < 0) {
        video_stream = -1;
    }

    if (video_stream < 0 && audio_stream < 0) {
        // No stream avliable
        signal_error.emit();
        return;
    }

    // Prepare codec
    auto codec = avcodec_find_decoder(container->streams[video_stream]->codecpar->codec_id);
    if (codec) {
        video_ctxt.reset(avcodec_alloc_context3(codec));
        if (!video_ctxt) {
            // Error
            signal_error.emit();
            return;
        }
        ret = avcodec_parameters_to_context(video_ctxt.get(), container->streams[video_stream]->codecpar);
        if (ret < 0) {
            // Error
            signal_error.emit();
            return;
        }
        video_codec = video_ctxt->codec;
        ret = avcodec_open2(video_ctxt.get(), video_codec, nullptr);
        if (ret < 0) {
            // Error
            signal_error.emit();
            return;
        }

        // Prepare buffer
        video_frame  = AVFramePtr::Alloc();
        video_frame2 = AVFramePtr::Alloc();

        video_cvt.reset(
            sws_getContext(
                video_ctxt->width,
                video_ctxt->height,
                video_ctxt->pix_fmt,
                video_ctxt->width,
                video_ctxt->height,
                AV_PIX_FMT_RGBA,
                0,
                nullptr,
                nullptr,
                nullptr
            )
        );
        assert(video_cvt);

        size_t n     = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_ctxt->width, video_ctxt->height, 32);
        uint8_t *buf = static_cast<uint8_t*>(av_malloc(n));

        av_image_fill_arrays(
            video_frame2->data,
            video_frame2->linesize,
            buf,
            AV_PIX_FMT_RGBA,
            video_ctxt->width, video_ctxt->height,
            32
        );
    }

    // All done, let start
    state = State::Playing;
    cond.notify_one();
}
void MediaPlayerImpl::pause() {
    state = State::Paused;
    cond.notify_one();
}
void MediaPlayerImpl::resume() {
    state = State::Playing;
    cond.notify_one();
}
void MediaPlayerImpl::stop() {
    state = State::Stopped;
    cond.notify_one();
}
bool MediaPlayerImpl::timer_event(TimerEvent &) {
    return true;
}

// VideoWidget
VideoWidget::VideoWidget(Widget *p) : Widget(p) {

}
VideoWidget::~VideoWidget() {

}
bool VideoWidget::paint_event(PaintEvent &) {
    auto &p = painter();
    p.set_color(background);
    p.fill_rect(rect());

    if (!texture.empty()) {
        // Keep aspect ratio
        FRect dst;
        FSize size = rect().size();
        FSize img_size = texture.size();

        if (img_size.w * size.h > img_size.h * size.w) {
            dst.w = size.w;
            dst.h = img_size.h * size.w / img_size.w;
        }
        else {
            dst.h = size.h;
            dst.w = img_size.w * size.h / img_size.h;
        }
        dst.x = rect().x + (size.w - dst.w) / 2;
        dst.y = rect().y + (size.h - dst.h) / 2;
        p.draw_image(texture, &dst, nullptr);
    }
    return true;
}
bool VideoWidget::begin(PixFormat *fmt) {
    *fmt = PixFormat::RGBA32;
    return true;
}
bool VideoWidget::end() {
    native_resolution = {0, 0};
    return true;
}
bool VideoWidget::write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) {
    assert(fmt == PixFormat::RGBA32);
    assert(ui_thread());

    native_resolution = {w, h};
    if (texture.empty() || texture.size() != native_resolution) {
        texture = painter().create_texture(fmt, w, h);
        // texture.set_interpolation_mode(InterpolationMode::Nearest);
    }
    texture.update(nullptr, data, pitch);

    repaint();
    return true;
}

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
    priv->resume();
}
void MediaPlayer::pause() {
    priv->pause();
}
void MediaPlayer::set_url(u8string_view url) {
    priv->video_url = url;
}
void MediaPlayer::set_video_output(AbstractVideoSurface *v) {
    priv->video_output = v;
}

BTK_NS_END