#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/plugins/media.hpp>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

// Import FFmpeg
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
}

// TODO : Improve sync algo
// TODO : Add audio sync
// FIXME : Miniaudio playback is broken
// FIXME : Memory leaks

// Avoid macro from windows.h
#undef max
#undef min

// Debug macro
#define PLAYER_LOG(...) BTK_LOG("[Player]" __VA_ARGS__)

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
        if (ptr) {
            Free(&ptr);
        }
    }

    template <typename T, auto Alloc, auto Free>
    using FFWrap = FFPtr<T, FFTraits<T, Alloc, Free>>;

    using AVFormatContextPtr = FFWrap<AVFormatContext, FFUnused<AVFormatContext>, FFPtr2Warp<AVFormatContext, avformat_close_input>>;
    using AVCodecContextPtr  = FFWrap<AVCodecContext,  FFUnused<AVCodecContext>,  avcodec_close>;
    using AVPacketPtr = FFWrap<AVPacket, av_packet_alloc, FFPtr2Warp<AVPacket, av_packet_free>>;
    using AVFramePtr  = FFWrap<AVFrame,  av_frame_alloc , FFPtr2Warp<AVFrame , av_frame_free>>;
    using SwsContextPtr = FFWrap<SwsContext, sws_getContext, sws_freeContext>;

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
        using PacketQueue = std::queue<AVPacketPtr>;
        using FrameQueue = std::queue<AVFramePtr>;
        using DataBuffer = std::vector<uint8_t>;

        MediaPlayerImpl();
        ~MediaPlayerImpl();

        void play();
        void stop();
        void pause();
        void resume();
        void set_position(double position);

        double duration() const;

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

        std::atomic<State>  state   = State::Stopped;
        std::atomic<bool>   running = true;

        std::atomic<double> position = 0.0;
        timestamp_t         position_start = 0;

        std::atomic<double> seek_position = 0.0;
        std::atomic<bool>   seek_req = false;

        int   video_stream = -1;
        int   audio_stream = -1;

        timestamp_t    video_ticks = 0; //< The time player write out the frame
        double         video_prev_pts = 0; //< Prev video timestamps
        bool           video_has_prev = false;
        bool           video_frame_unused = false; //< Because of sync, prev frame was decoded, but not used
        PacketQueue    video_packet_queue;
        std::mutex     video_mtx; //< For Protect video_frame2

        DataBuffer     audio_buffer;
        FrameQueue     audio_frame_queue;
        bool           audio_device_inited = false;
        bool           audio_frame_encough = false;
        size_t         audio_buffer_used = 0;
        std::mutex     audio_mtx; //< For Protect audio frame queue

        AbstractAudioDevice* audio_device = nullptr;
        std::atomic<double>  audio_pts = 0;

        int            errcode = 0;

        Signal<void(double)> signal_duration_changed;
        Signal<void(double)> signal_position_changed;
        Signal<void()> signal_played;
        Signal<void()> signal_paused;
        Signal<void()> signal_stopped;
        Signal<void()> signal_error;

        void codec_thread();
        void clock_update();

        bool video_init(); //< Init env for video stream
        void video_send(); //< Send packet
        bool video_proc(); //< Process frame (false on no frame)
        bool video_decode();
        void video_cleanup();

        bool audio_init();
        void audio_send(); //< Send packet
        bool audio_proc(); //< Process frame (false on no frame)
        void audio_callback(void* out, uint32_t frame);
        bool audio_fillbuffer();
        bool audio_decode();

        // AudioDevice
        void audio_device_init();
        void audio_device_destroy();
        void audio_device_pause(bool v);
        bool audio_device_is_paused();
        void audio_cache_clear();
};

// Begin video player
MediaPlayerImpl::MediaPlayerImpl() {
    thrd = std::thread(&MediaPlayerImpl::codec_thread, this);
}
MediaPlayerImpl::~MediaPlayerImpl() {
    audio_device_destroy();

    state = State::Stopped;
    running = false;
    cond.notify_one();
    thrd.join();

    video_cleanup();
}

void MediaPlayerImpl::codec_thread() {
    int ret;
    while (running) {
        if (state != State::Playing) {
            if (state == State::Stopped) {
                position = 0;
                position_start = 0;
                audio_pts = 0;

                if (!signal_position_changed.empty()) {
                    defer_call(&Signal<void(double)>::emit, &signal_position_changed, 0.0);
                }
                video_cleanup();
            }

            // Wait for sth changed
            std::unique_lock lk(mutx);
            cond.wait(lk, [this]() {
                return !running || state == State::Playing;
            });

            if (state == State::Playing) {
                // Restore timestamp
                position_start = GetTicks() - position * 1000;
                BTK_LOG("[Video thread] Restore to Playing mode, position %lf, position_start %ld\n", position.load(), position_start);
            }
        }
        if (!container) {
            continue;
        }
        // FIXME : Process last frame if
        while ((ret = av_read_frame(container.get(), packet.get())) >= 0 && state == State::Playing) {
            // Update position
            clock_update();
            // Process
            if (packet->stream_index == video_stream) {
                video_send();
                video_proc();
            }
            else if (packet->stream_index == audio_stream) {
                audio_send();
                audio_proc();
            }
            av_packet_unref(packet.get());

            
            // Check seek request
            if (seek_req) {
                int ret;

                // Codec Flush
                if (video_stream >= 0) {
                    auto time_base = av_q2d(container->streams[video_stream]->time_base);
                    ret = av_seek_frame(container.get(), video_stream, seek_position / time_base, AVSEEK_FLAG_BACKWARD);

                    video_cleanup();
                    avcodec_flush_buffers(video_ctxt.get());
                }
                if (audio_stream >= 0) {
                    auto time_base = av_q2d(container->streams[audio_stream]->time_base);
                    ret = av_seek_frame(container.get(), audio_stream, seek_position / time_base, AVSEEK_FLAG_BACKWARD);

                    bool paused = audio_device_is_paused();
                    if (!paused) {
                        audio_device_pause(true);
                    }

                    audio_cache_clear();
                    avcodec_flush_buffers(audio_ctxt.get());

                    audio_pts = position.load();

                    if (!paused) {
                        audio_device_pause(false);
                    }
                }

                position = seek_position.load();
                seek_req = false;

                position_start = GetTicks() - position * 1000;
                continue;
            }
        }

        if (ret == AVERROR_EOF) {
            // AVEof 
            PLAYER_LOG("[Codec thread] Stream to Eof \n");
            while (position < duration() && (video_proc() || audio_proc())) {
                clock_update();
            }
            PLAYER_LOG("[Codec thread] Queue to Eof \n");
        }

        // EOF
        if (state == State::Playing) {
            state = State::Stopped;

            audio_device_destroy();
            // Notify
            if (video_output) {
                defer_call(&AbstractVideoSurface::end, video_output);
            }
            defer_call(&Signal<void()>::emit, &signal_stopped);
        }
    }
}
void MediaPlayerImpl::video_send() {
    if (!video_output || !video_ctxt) {
        // No need to proc, drop
        return;
    }
    // Push packet to queue
    AVPacketPtr pack = AVPacketPtr::Alloc();
    av_packet_ref(pack.get(), packet.get());
    av_packet_unref(packet.get());

    // Realease the mem
    video_packet_queue.emplace(std::move(pack));
}
bool MediaPlayerImpl::video_proc() {
    // Try run out the packet
    while (video_decode()) {
        clock_update();

        int ret;

        // Unpack info
        auto time_base = av_q2d(container->streams[video_stream]->time_base);
        auto pts = video_frame->pts * time_base;
        auto ticks = GetTicks();

        assert(time_base != 0.0);

        if (video_has_prev) {
            // FIXME : Sync video to audio, this way is little dirty
            if (position > pts + 0.1) {
                // audio is quicker
                PLAYER_LOG("[Video codec] Too last, drop %c frame %lf\n", av_get_picture_type_char(video_frame->pict_type) ,pts);
                PLAYER_LOG("[Video codec] Player position %lf\n", position.load());
                PLAYER_LOG("[Video codec] audio position %lf\n", audio_pts.load());
                video_has_prev = false;
                continue;
            }
            // Convert to ms
            auto pts_diff = (pts - video_prev_pts) * 1000;
            auto ticks_diff = ticks - video_ticks;
            auto diff = pts_diff - ticks_diff;

            if (diff > 0.01) {
                // We need sleep
                if (!audio_frame_encough) {
                    video_frame_unused = true;
                    return true;
                }
                // Encough audio frame, we can sleep now
                int64_t ms = diff;

                // Wait, exception play status changed & no audio frame
                std::unique_lock lk(mutx);
                cond.wait_for(lk, std::chrono::milliseconds(ms));
                clock_update();

                if (!audio_frame_encough) {
                    video_frame_unused = true;
                    return true;
                }
                if (state != State::Playing || seek_req) {
                    video_has_prev = false;
                    return true;
                }
            }
            else if (diff < 1.0 / 30) {
                video_has_prev = false;
                PLAYER_LOG("[Video codec] Too last, drop frame %lf\n", pts);
                PLAYER_LOG("[Video codec] Player position %lf\n", position.load());
                PLAYER_LOG("[Video codec] audio position %lf\n", audio_pts.load());
                continue;
            }
        }

        // PLAYER_LOG("[Video codec] video frame pts %lf\n", pts);

        video_frame_unused = false;
        // Process frame
        video_mtx.lock();

        // av_hwframe_transfer_data(video_frame.get(), ;
        ret = sws_scale(
            video_cvt.get(),
            video_frame->data,
            video_frame->linesize,
            0,
            video_ctxt->height,
            video_frame2->data,
            video_frame2->linesize
        );
        // ret = sws_scale_frame(video_cvt.get(), video_frame2.get(), video_frame.get());
        video_mtx.unlock();

        // Push to surface
        video_ticks = GetTicks();
        video_prev_pts = pts;
        defer_call([this]() {
            std::lock_guard locker(video_mtx);
            video_output->write(PixFormat::RGBA32, video_frame2->data[0], video_frame2->linesize[0], video_ctxt->width, video_ctxt->height);
        });

        video_has_prev = true;
    }
    return false;
}
bool MediaPlayerImpl::video_decode() {
    if (video_frame_unused) {
        return true;
    }

    int ret;

    do {
        ret = avcodec_receive_frame(video_ctxt.get(), video_frame.get());
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            if (video_packet_queue.empty()) {
                // No Packet
                return false;
            }

            // Need packet
            ret = avcodec_send_packet(video_ctxt.get(), video_packet_queue.front().get());
            if (ret != 0) {
                // Error
                return false;
            }
            av_packet_unref(video_packet_queue.front().get());
            video_packet_queue.pop();
            continue;
        }
        else if (ret != 0) {
            // Codec Error
            return false;
        }
        else {
            return true;
        }
    }
    while(!video_packet_queue.empty());
    // No Packet
    return false;
}
void MediaPlayerImpl::video_cleanup() {
    while (!video_packet_queue.empty()) {
        av_packet_unref(video_packet_queue.front().get());
        video_packet_queue.pop();
    }
    // Cleanup prev
    video_ticks = 0;
    video_prev_pts = 0.0;
    video_has_prev = false;
}
void MediaPlayerImpl::clock_update() {
    double new_position = double(GetTicks() - position_start) / 1000;

    // Emit signal if
    if (int64_t(new_position) != int64_t(position.load())) {
        if (!signal_position_changed.empty()) {
            defer_call(&Signal<void(double)>::emit, &signal_position_changed, position.load());
        }
    }

    position = new_position;
}
void MediaPlayerImpl::audio_send() {
    if (!audio_ctxt || audio_stream < 0) {
        return;
    }

    int ret = avcodec_send_packet(audio_ctxt.get(), packet.get());
    if (ret != 0) {
        // Error
        return;
    }
}
bool MediaPlayerImpl::audio_proc() {
    if (!audio_ctxt || audio_stream < 0) {
        return false;
    }
    int ret = avcodec_receive_frame(audio_ctxt.get(), audio_frame.get());
    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
        // We need more packet
        return !audio_frame_queue.empty();
    }

    auto time_base = av_q2d(container->streams[audio_stream]->time_base);
    auto pts = audio_frame->pts * time_base;

    // Got frame, push to audio player thread
    std::lock_guard locker(audio_mtx);
    audio_frame_queue.push(std::move(audio_frame));
    audio_frame = AVFramePtr::Alloc();

    audio_frame_encough = audio_frame_queue.size() > 200;

    // Done
    av_packet_unref(packet.get());

    return !audio_frame_queue.empty();
}
// bool MediaPlayerImpl::audio_decode() {
//     int ret = avcodec_receive_frame(audio_ctxt.get(), audio_frame.get());
//     while () {

//     }
// }
void MediaPlayerImpl::play() {
    if (state != State::Stopped) {
        stop();
    }
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
        errcode = ret;
        signal_error.emit();
        return;
    }
    container.reset(ps);

    ret = avformat_find_stream_info(container.get(), nullptr);
    if (ret != 0) {
        // Error
        errcode = ret;
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
        audio_stream = -1;
    }

    if (video_stream < 0 && audio_stream < 0) {
        // No stream avliable
        signal_error.emit();
        return;
    }

    // Prepare codec
    if (video_stream >= 0){
        if (!video_init()) {
            return;
        }
    }
    if (audio_stream >= 0) {
        if (!audio_init()) {
            return;
        }
    }

    if (duration() != 0.0) {
        defer_call(&Signal<void(double)>::emit, &signal_duration_changed, duration());
    }

    // All done, let start
    state = State::Playing;
    cond.notify_one();
}

bool MediaPlayerImpl::video_init() {
    const AVCodec *codec;
    int ret;

    codec = avcodec_find_decoder(container->streams[video_stream]->codecpar->codec_id);

    if (!codec) {
        // No codec
        return false;
    }

    video_ctxt.reset(avcodec_alloc_context3(codec));
    if (!video_ctxt) {
        // Error
        errcode = AVERROR(ENOMEM);
        signal_error.emit();
        return false;
    }
    ret = avcodec_parameters_to_context(video_ctxt.get(), container->streams[video_stream]->codecpar);
    if (ret < 0) {
        // Error
        errcode = ret;
        signal_error.emit();
        return false;
    }
    video_codec = video_ctxt->codec;
    ret = avcodec_open2(video_ctxt.get(), video_codec, nullptr);
    if (ret < 0) {
        // Error
        errcode = ret;
        signal_error.emit();
        return false;
    }

    PLAYER_LOG("[Video codec] %s\n", video_ctxt->codec_descriptor->long_name);

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

    // Cleanup prev
    video_ticks = 0;
    video_prev_pts = 0.0;
    video_has_prev = false;

    // Notify video output if
    if (video_output) {
        PixFormat fmt = PixFormat::RGBA32;
        video_output->begin(&fmt);
        
        assert(fmt == PixFormat::RGBA32);
    }
    return true;
}


bool MediaPlayerImpl::audio_init() {
    const AVCodec *codec;
    int ret;

    codec = avcodec_find_decoder(container->streams[audio_stream]->codecpar->codec_id);
    if (!codec) {
        return false;
    }
    audio_ctxt.reset(avcodec_alloc_context3(codec));
    if (!audio_ctxt) {
        // Error
        errcode = AVERROR(ENOMEM);
        signal_error.emit();
        return false;
    }
    ret = avcodec_parameters_to_context(audio_ctxt.get(), container->streams[audio_stream]->codecpar);
    if (ret < 0) {
        // Error
        errcode = ret;
        signal_error.emit();
        return false;
    }
    audio_codec = audio_ctxt->codec;
    ret = avcodec_open2(audio_ctxt.get(), audio_codec, nullptr);
    if (ret < 0) {
        // Error
        errcode = ret;
        signal_error.emit();
        return false;
    }

    // Prepare audio device
    audio_device_init();

    return true;
}
void MediaPlayerImpl::audio_callback(void *out, uint32_t byte) {
    // byte = 2 * byte * sizeof(int32_t);

    // // First check buffer has encough data
    // size_t avliable = audio_buffer.size() - audio_buffer_used;
    // size_t ncopyed  = min<size_t>(avliable, byte);

    // Btk_memcpy(out, audio_buffer.data(), ncopyed);

    // // Move 
    // out = static_cast<uint8_t*>(out) + ncopyed;
    // audio_buffer_used += ncopyed;
    // byte -= ncopyed;

    // if (byte) {
    //     // Still has data need to
    //     std::lock_guard locker(audio_mtx);
    //     if (!audio_frame_queue.empty()) {

    //     }
    // }
    // if (byte) {
    //     // No more data :(
    //     Btk_memset(out, 0, byte);
    // }
#if !defined(NDEBUG)
    static double prev_pts = 0;
#endif

    while (byte > 0 && state == State::Playing) {
        // First check buffer has encough data
        size_t avliable = audio_buffer.size() - audio_buffer_used;
        size_t ncopyed  = min<size_t>(avliable, byte);

        Btk_memcpy(out, audio_buffer.data(), ncopyed);

        // Move 
        out = static_cast<uint8_t*>(out) + ncopyed;
        byte -= ncopyed;
        audio_buffer_used += ncopyed;

        audio_pts = audio_pts + double(ncopyed) / (2 * audio_ctxt->channels * audio_ctxt->sample_rate);

        if (byte) {
            // We need more data
            if (!audio_fillbuffer()) {
                // No more data :(
                break;
            }
        }

#if !defined(NDEBUG)
        if (prev_pts != audio_pts) {
            prev_pts = audio_pts;
            // PLAYER_LOG("[Audio Thread] audio pts %lf\n", prev_pts);
        }
#endif
    }
    if (byte) {
        // Make silence
        // PLAYER_LOG("[Audio Thread] make %u byte data into slience\n", byte);
        Btk_memset(out, 0, byte);
    }
}
bool MediaPlayerImpl::audio_fillbuffer() {
    std::unique_lock locker(audio_mtx);
    while (audio_frame_queue.empty()) {

        // Try wait for codec thread
        audio_frame_encough = false;
        locker.unlock();
        cond.notify_one();
        locker.lock();

        if (seek_req) {
            // Seeking, we make it slience
            return false;
        }
    }
    // Resampling
    auto frame = audio_frame_queue.front().get();

    // Buffer should be fully used
    // assert(audio_buffer_used == audio_buffer.size());
    if (audio_buffer_used != audio_buffer.size()) {
        PLAYER_LOG("[Audio Thread] Warning audio_buffer_used != audio_buffer.size()\n");
    }

    // Setting up 
    int64_t in_channal_layout = (audio_ctxt->channels ==
                        av_get_channel_layout_nb_channels(audio_ctxt->channel_layout)) ?
                        audio_ctxt->channel_layout :
                        av_get_default_channel_layout(audio_ctxt->channels);
    

    auto    out_channal_layout = in_channal_layout;
    auto    out_sample_fmt     = AV_SAMPLE_FMT_S32;
    auto    out_channals       = audio_ctxt->channels;

    if (in_channal_layout < 0) {
        audio_frame_queue.pop();
        return false;
    }

    SwrContext *cvt = swr_alloc();

    // Set convert args
    av_opt_set_int(cvt, "in_channel_layout", in_channal_layout, 0);
    av_opt_set_int(cvt, "in_sample_rate", audio_ctxt->sample_rate, 0);
    av_opt_set_sample_fmt(cvt, "in_sample_fmt", audio_ctxt->sample_fmt, 0);

    av_opt_set_int(cvt, "out_channel_layout", out_channal_layout, 0);
    av_opt_set_int(cvt, "out_sample_rate", audio_ctxt->sample_rate, 0);
    av_opt_set_sample_fmt(cvt, "out_sample_fmt", out_sample_fmt, 0);

    if (swr_init(cvt) < 0) {
        swr_free(&cvt);
        audio_frame_queue.pop();
        return true;
    }

    auto nsamples = av_rescale_rnd(
        frame->nb_samples,
        audio_ctxt->sample_rate,
        audio_ctxt->sample_rate,
        AV_ROUND_UP
    );
    assert(nsamples > 0);

    int samples_size;
    int ret = av_samples_get_buffer_size(
        &samples_size,
        out_channals,
        nsamples,
        out_sample_fmt,
        1
    );
    assert(ret >= 0);

    audio_buffer.resize(samples_size);
    audio_buffer_used = 0;

    // Begin convert
    uint8_t *outaddr = audio_buffer.data();

    ret = swr_convert(
        cvt,
        &outaddr,
        audio_buffer.size(),
        (const uint8_t**)frame->data,
        frame->nb_samples
    );

    assert(ret >= 0);
    
    audio_pts = frame->pts * av_q2d(container->streams[audio_stream]->time_base);

    // Done
    swr_free(&cvt);
    audio_frame_queue.pop();
    return true;
}
void MediaPlayerImpl::audio_device_init() {
    if (audio_device) {
        audio_device->bind([this](void *b, uint32_t s) {
            audio_callback(b, s);
        });
        audio_device->open(
            AudioSampleFormat::Sint32,
            audio_ctxt->sample_rate,
            audio_ctxt->channels
        );
        audio_device->pause(false);
        audio_device_inited = true;
    }
}
void MediaPlayerImpl::audio_device_pause(bool v) {
    if (audio_device_inited) {
        audio_device->pause(v);
    }
}
bool MediaPlayerImpl::audio_device_is_paused() {
    return audio_device->is_paused();
}
void MediaPlayerImpl::audio_device_destroy() {
    if (audio_device_inited) {
        audio_device->close();
        audio_device_inited = false;
        audio_pts = 0;

        audio_cache_clear();
    }
}
void MediaPlayerImpl::audio_cache_clear() {
    while (!audio_frame_queue.empty()) {
        audio_frame_queue.pop();
    }

    audio_buffer.clear();
    audio_buffer_used = 0;
}
void MediaPlayerImpl::pause() {
    audio_device_pause(true);

    state = State::Paused;
    cond.notify_one();
}
void MediaPlayerImpl::resume() {
    audio_device_pause(false);

    state = State::Playing;
    cond.notify_one();
}
void MediaPlayerImpl::stop() {
    state = State::Stopped;
    cond.notify_one();

    audio_device_destroy();

    if (video_output) {
        video_output->end();
    }
}

void MediaPlayerImpl::set_position(double pos) {
    // TODO : 
    // if (!container) {
    //     return;
    // }

    // int64_t where = pos * AV_TIME_BASE;
    // State s = state.load();

    // where = clamp<int64_t>(pos, 0, container->duration);

    // pause();


    // if (audio_stream >= 0) {
    //     if (av_seek_frame(container.get(), audio_stream, where, AVSEEK_FLAG_BACKWARD) < 0) {
    //         abort();
    //     }
    // }
    // if (video_stream) {
    //     if (av_seek_frame(container.get(), video_stream, where, AVSEEK_FLAG_BACKWARD) < 0) {
    //         abort();
    //     }
    // }

    // position = pos;

    // // Resume
    // state = s;
    // cond.notify_one();
    seek_position = pos;
    seek_req = true;

    cond.notify_one();
}

double MediaPlayerImpl::duration() const {
    if (container) {
        return double(container->duration) / AV_TIME_BASE;
    }
    return 0.0;
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
    playing = true;
    return true;
}
bool VideoWidget::end() {
    playing = false;
    texture.clear();
    repaint();
    return true;
}
bool VideoWidget::write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) {
    assert(fmt == PixFormat::RGBA32);
    assert(ui_thread());

    if (!playing) {
        return false;
    }

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
void MediaPlayer::set_audio_output(AbstractAudioDevice *a) {
    priv->audio_device = a;
}
void MediaPlayer::set_position(double pos) {
    priv->set_position(pos);
}

auto MediaPlayer::signal_duration_changed() -> Signal<void(double)> & {
    return priv->signal_duration_changed;
}
auto MediaPlayer::signal_position_changed() -> Signal<void(double)> & {
    return priv->signal_position_changed;
}
auto MediaPlayer::signal_error() -> Signal<void()> & {
    return priv->signal_error;
}

double MediaPlayer::duration() const {
    return priv->duration();
}
double MediaPlayer::position() const {
    return priv->position;
}
u8string MediaPlayer::error_string() const {
    return format_fferror(priv->errcode);
}

BTK_NS_END