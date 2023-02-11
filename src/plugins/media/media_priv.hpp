#pragma once

#include <Btk/detail/threading.hpp>
#include <Btk/plugins/media.hpp>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>

#include "ffmpeg_utils.hpp"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>

    #include <libavcodec/avcodec.h>
}

// Need we add std::atomic to audio clock

BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

// Useful const
inline constexpr auto AVSyncThreshold = 0.01;
inline constexpr auto AVNoSyncThreshold = 1.0;
inline constexpr auto AudioDiffAvgNB = 20;

// inline constexpr auto MaxAudioPacketSize = 200;
// inline constexpr auto MaxVideoPacketSize = 200;

// Special packet
inline constexpr auto EofPacket = nullptr;
inline static auto FlushPacket = reinterpret_cast<AVPacket*>(0x01);
inline static auto StopPacket = reinterpret_cast<AVPacket*>(0x02);

inline bool IsSpecialPacket(AVPacket *pak) noexcept {
    return pak == EofPacket || pak == FlushPacket || pak == StopPacket;
}

enum class SyncType : uint8_t {
    AudioMaster, // default by audio
    VideoMaster,
    ExternalMaster,
};


class Demuxer;

class PacketQueue {
    public:
        PacketQueue();
        PacketQueue(const PacketQueue &) = delete;
        ~PacketQueue();

        void flush();
        void put(AVPacket *packet);
        auto get(bool blocking = true) -> AVPacket *;
        size_t size();
        int64_t duration();
    private:
        std::queue<AVPacket*> packets;
        std::condition_variable cond;
        std::mutex              mutex;
        std::mutex              cond_mutex;
        int64_t                 packets_duration = 0; //< Sums of packet duration
};

class Clock {
    public:
        Clock() = default;
        ~Clock() = default;

        double get();
        void   pause(bool v);
    private:
        double  pts = 0.0;
        double  pts_drift = 0.0;
};

/**
 * @brief Video thread for decode
 * 
 */
class VideoThread : public Object {
    public:
        VideoThread(Demuxer *demuxer, AVStream *stream, AVCodecContext *ctxt);
        ~VideoThread();

        // No work to do
        bool running() {
            return !nothing_to_do;
        }
        void pause(bool v);
        PacketQueue *packet_queue() {
            return &queue;
        }

        double clock_time() const {
            return video_clock;
        }
        bool is_paused() const {
            return paused;
        }
    private:
        void thread_main();
        bool video_decode_frame(AVPacket *packet);

        void video_surface_ui_callback();
        void video_clock_pause(bool v);

        Demuxer *demuxer = nullptr;
        AVStream *stream = nullptr;
        AVCodecContext *ctxt = nullptr;
        AbstractVideoSurface *surface;
        PacketQueue queue;

        // Frame
        AVPtr<SwsContext> sws_ctxt;
        AVPtr<AVFrame> src_frame {av_frame_alloc()};
        AVPtr<AVFrame> dst_frame {av_frame_alloc()};
        std::mutex dst_frame_mtx;

        // Hardware
        AVPtr<AVFrame> sw_frame {av_frame_alloc()};
        AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

        // Sync 
        std::thread thrd;
        std::condition_variable cond;
        std::mutex cond_mtx;
        bool paused = false;

        // Status
        int64_t video_clock_start = 0.0f; //< Video started time
        double  video_clock = 0.0f;
        bool nothing_to_do = false;
};

/**
 * @brief Audio callbacl thread
 * 
 */
class AudioThread {
    public:
        AudioThread(Demuxer *demuxer, AVStream *stream, AVCodecContext *ctxt);
        ~AudioThread();

        // No work to do
        bool running() {
            return !nothing_to_do;
        }
        PacketQueue *packet_queue() {
            return &queue;
        }

        bool is_paused() {
            return device->is_paused();
        }
        void pause(bool v) {
            return device->pause(v);
        }

        double clock_time() {
            return audio_clock;
        }
    private:
        void audio_callback(void *data, int len);
        int  audio_decode_frame();
        int  audio_resample(int out_samples);
        /**
         * @brief Get samples size by it
         * 
         * @param nb_samples 
         * @return int 
         */
        int  audio_sync(int nb_samples);


        AbstractAudioDevice *device;
        PacketQueue queue;
        Demuxer *demuxer = nullptr;
        AVStream *stream = nullptr;
        AVCodecContext *ctxt = nullptr;

        // Frame decode to
        AVPtr<AVFrame> frame {av_frame_alloc()};
        AVPtr<SwrContext> swr_ctxt{ };
        std::vector<uint8_t> buffer; //< Tmp buffer
        int buffer_index = 0; //< Position in buffer, (in byte)

        // Status
        bool nothing_to_do = false;
        double audio_clock = 0.0f;

        // Sync status
        double audio_diff_cum = 0.0;
        double audio_diff_avg_coef = exp(log(0.01) / AudioDiffAvgNB);
        double audio_diff_threshold = AVSyncThreshold;
        int    audio_diff_avg_count = 0;

};

/**
 * @brief Demuxer, read data and demux into packet to send it
 * 
 */
class Demuxer : public Object {
    public:
        using State = MediaPlayer::State;

        Demuxer();
        ~Demuxer();

        void play();
        void pause(bool v);
        void stop();

        void load();
        void unload();

        bool is_loaded();

        void set_url(u8string_view url);
        void set_position(double pos);

        bool seekable();
        double duration();
        double position();

        // Get buffered, in seconds
        double buffered();

        // Get ffmpeg error code
        int    error_code();

        double master_clock();
        SyncType master_sync_type();

        AbstractVideoSurface *video_output() const {
            return video_surf;
        }
        AbstractAudioDevice  *audio_output() const {
            return audio_device;
        }
        void set_video_output(AbstractVideoSurface *v) {
            video_surf = v;
        }
        void set_audio_output(AbstractAudioDevice *a) {
            audio_device = a;
        }

        BTK_EXPOSE_SIGNAL(_media_status_changed);
        BTK_EXPOSE_SIGNAL(_duration_changed);
        BTK_EXPOSE_SIGNAL(_position_changed);
        BTK_EXPOSE_SIGNAL(_state_changed);
        BTK_EXPOSE_SIGNAL(_error);
    private:
        void thread_main();
        void handle_seek();
        bool open_component(int stream_idx);
        /**
         * @brief Demuxer
         * 
         * @return true State changed
         * @return false No change
         */
        bool demuxer_wait_state_change(std::chrono::microseconds ms);
        void clock_update();
        void clock_pause(bool v);
        void set_state(State state);
        void set_status(MediaStatus status);
        /**
         * @brief Internal ffmpeg read callback
         * 
         * @return int 
         */
        int  interrupt_cb();

        // Format Context
        AVFormatContext *ctxt = nullptr;
        bool             quit  = false;
        bool             loaded = false;
        std::thread      demuxer_thread;
        std::mutex       demuxer_mtx;
        std::condition_variable demuxer_cond; //< Cond for something changed

        // Clock for position
        int64_t          clock_start = 0; //< time of clock started
        double           clock    = 0.0; //< Current position
        bool             clock_paused = false;
        SyncType         sync_type = SyncType::AudioMaster;

        // Status 
        u8string         url;
        int              av_code = 0;
        MediaStatus      status = MediaStatus::NoMedia;
        State            state = State::Stopped;

        // Buffering     
        int              buffered_packets_limit = 1000;
        double           buffered_user_hint     = 0.0; //< hint to buffer in user_hint seconds


        // Seek 
        bool             seek_req = false;
        double           seek_pos = 0.0;

        // Audio
        AbstractAudioDevice *audio_device = nullptr;
        AudioThread *audio_thread = nullptr;
        int          audio_stream = -1;

        // Video
        AbstractVideoSurface *video_surf = nullptr;
        VideoThread *video_thread = nullptr;
        int          video_stream = -1;

        // Signals
        Signal<void(double)> _duration_changed;
        Signal<void(double)> _position_changed;
        Signal<void()>       _error;
        Signal<void(State)>  _state_changed;
        Signal<void(MediaStatus)> _media_status_changed;
};


// Useful func
void SetThreadDescription(const char *name);

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)