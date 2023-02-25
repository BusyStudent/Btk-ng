#include "build.hpp"
#include "media_priv.hpp"

extern "C" {
    #include <libavutil/time.h>
    #include <libavutil/dict.h>
}


BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

using namespace std::chrono_literals;

Demuxer::Demuxer() {
    // av_log_set_level(AV_LOG_TRACE);
}
Demuxer::~Demuxer() {
    stop();
    avformat_close_input(&ctxt);

    av_dict_free(&options);
}

void Demuxer::stop() {
    if (!loaded) {
        return;
    }

    // Tell video & audio & demuxer thread to stop
    quit = true;
    demuxer_cond.notify_one();

    // Waiting
    demuxer_thread.join();

    avformat_close_input(&ctxt);

    // Reset value
    ctxt = nullptr;
    quit = false;
    loaded = false;

    set_state(State::Stopped);
    set_status(MediaStatus::NoMedia);

    
    // Emit signals
    clock = 0;

    // signal_duration_changed().emit(0);
    signal_position_changed().emit(0);
}
void Demuxer::load() {
    if (loaded) {
        return;
    }

    ctxt = avformat_alloc_context();
    if (!ctxt) {
        return;
    }

    ctxt->interrupt_callback.opaque = this;
    ctxt->interrupt_callback.callback = [](void *self) {
        return static_cast<Demuxer*>(self)->interrupt_cb();
    };

    set_status(MediaStatus::LoadingMedia);
    av_code = avformat_open_input(
        &ctxt,
        url.c_str(),
        nullptr,
        &options
    );

    if (av_code < 0) {
        set_status(MediaStatus::NoMedia);
        signal_error().emit();
        return;
    }

    av_code = avformat_find_stream_info(ctxt, nullptr);
    if (av_code < 0) {
        set_status(MediaStatus::NoMedia);
        signal_error().emit();
        return;
    }
    // Try find stream
    video_stream = av_find_best_stream(ctxt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audio_stream = av_find_best_stream(ctxt, AVMEDIA_TYPE_AUDIO, -1, video_stream, nullptr, 0);

    // Ok
    loaded = true;

    set_status(MediaStatus::LoadedMedia);

    // Emit signals
    signal_duration_changed().emit(ctxt->duration / double(AV_TIME_BASE));

    // Debug
#if !defined(NDEBUG)
    av_dump_format(ctxt, 0, url.c_str(), 0);
#endif
}
void Demuxer::unload() {
    // TODO : 
}
void Demuxer::play() {
    // Clear prev
    stop();
    load();

    if (!loaded) {
        return;
    }

    // Check output
    if (!audio_device) {
        audio_stream = -1;
    }
    if (!video_surf) {
        video_stream = -1;
    }

    if (video_stream >= 0) {
        if (!open_component(video_stream)) {
            // Fail
            video_stream = -1;
        }
    }
    if (audio_stream >= 0) {
        if (!open_component(audio_stream)) {
            // Fail
            audio_stream = -1;
        }
    }

    if (video_stream < 0 && audio_stream < 0) {
        // No Stream 
        av_code = AVERROR_STREAM_NOT_FOUND;
        signal_error().emit();
        return;
    }

    // Let's start
    set_state(State::Playing);
    demuxer_thread = std::thread(&Demuxer::thread_main, this);
}
void Demuxer::thread_main() {
    SetThreadDescription("DemuxerThread");

    AVPacket *packet = av_packet_alloc();
    int  ret = 0;
    bool eof = false;

    // Start clock
    clock_start = av_gettime_relative();
    clock = 0;

    while (!quit) {
        // Update clock, Label for break the double for
        mainloop : clock_update();

        // Check seek here
        if (seek_req) {
            handle_seek();
            eof = false;
        }

        // Check pause here
        if (state == State::Paused) {
            if (audio_thread) {
                audio_thread->pause(true);
            }
            if (video_thread) {
                video_thread->pause(true);
            }

            // Save clock
            clock_pause(true);

            // Wait if paused
            while (state == State::Paused && !quit) {
                if (demuxer_wait_state_change(1h)) {
                    continue;
                }
            }

            // End of pause, restore clock
            clock_pause(false);

            if (audio_thread) {
                audio_thread->pause(false);
            }
            if (video_thread) {
                video_thread->pause(false);
            }
        }


        // Check too much packet
        if (video_thread) {
            if (video_thread->packet_queue()->size() > buffered_packets_limit) {
                if (demuxer_wait_state_change(20ms)) {
                    continue;
                }
            }
        }
        if (audio_thread) {
            if (audio_thread->packet_queue()->size() > buffered_packets_limit) {
                if (demuxer_wait_state_change(20ms)) {
                    continue;
                }
            }
        }

        // Begin read
        ret = av_read_frame(ctxt, packet);

        // Process err
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                eof = true;
            }
            
        }
        else {
            // Add Packet
            if (packet->stream_index == audio_stream) {
                audio_thread->packet_queue()->put(av_packet_clone(packet));
            }
            else if (packet->stream_index == video_stream) {
                video_thread->packet_queue()->put(av_packet_clone(packet));
            }

            av_packet_unref(packet);
        }

        // Wait for thread finish it's job
        if (eof) {
            // Has Audio thread
            if (audio_thread) {
                // Tell we didnot has more packet
                audio_thread->packet_queue()->put(EofPacket);
                while (audio_thread->running() && !quit) {
                    if (demuxer_wait_state_change(10ms)) {
                        // Something changed
                        goto mainloop;
                    }
                }
                // Finished
            }
            if (video_thread) {
                // Tell we didnot has more packet
                video_thread->packet_queue()->put(EofPacket);
                while (video_thread->running() && !quit) {
                    if (demuxer_wait_state_change(10ms)) {
                        // Something changed
                        goto mainloop;
                    }
                }
                // Finished
            }

            // All done
            quit = true;
        }
    }

    // Cleanup video thread & audio thread
    if (video_thread) {
        // STOP
        video_thread->packet_queue()->flush();
        video_thread->packet_queue()->put(StopPacket);
        delete video_thread;
        video_thread = nullptr;

        video_stream = -1;
    }

    if (audio_thread) {
        // STOP
        audio_thread->packet_queue()->flush();
        audio_thread->packet_queue()->put(StopPacket);
        delete audio_thread;
        audio_thread = nullptr;

        audio_stream = -1;
    }

    av_packet_free(&packet);

    set_status(MediaStatus::EndOfMedia);
    set_state(State::Stopped);
}

int  Demuxer::interrupt_cb() {

    // Quit if asked
    return quit;
}

bool Demuxer::demuxer_wait_state_change(std::chrono::microseconds ms) {
    clock_update();

    std::unique_lock lock(demuxer_mtx);
    if (demuxer_cond.wait_for(lock, ms) == std::cv_status::no_timeout) {
        // State changed
        return true;
    }
    return false;
}
bool Demuxer::open_component(int stream_index) {
    AVStream *stream = ctxt->streams[stream_index];
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        // No codec
        av_code = AVERROR_DECODER_NOT_FOUND;
        return false;
    }
    AVCodecContext *codectxt = avcodec_alloc_context3(codec);
    if (!codec) {
        av_code = AVERROR(ENOMEM);
        return false;
    }
    av_code = avcodec_parameters_to_context(codectxt, stream->codecpar);
    if (av_code < 0) {
        avcodec_free_context(&codectxt);
        return false;
    }
    av_code = avcodec_open2(codectxt, codectxt->codec, nullptr);
    if (av_code < 0) {
        avcodec_free_context(&codectxt);
        return false;
    }

    // Common init done
    if (codectxt->codec_type == AVMEDIA_TYPE_AUDIO) {
        BTK_TRY {
            audio_thread = new AudioThread(this, stream, codectxt);
            return true;
        }
        BTK_CATCH (std::exception &err) {
            BTK_LOG("Failed to create audio thread %s\n", err.what());
        }
    }
    else if (codectxt->codec_type == AVMEDIA_TYPE_VIDEO) {
        BTK_TRY {
            video_thread = new VideoThread(this, stream, codectxt);
            return true;
        }
        BTK_CATCH (std::exception &err) {
            BTK_LOG("Failed to create video thread %s\n", err.what());
        }
    }

    // Error or Unknown
    avcodec_free_context(&codectxt);
    av_code = AVERROR_UNKNOWN;
    return false;
}
void Demuxer::clock_update() {
    if (clock_paused) {
        return;
    }

    double new_position = 0;
    if (sync_type == SyncType::ExternalMaster) {
        new_position = (av_gettime_relative() - clock_start) / 1000000.0;
    }
    else {
        new_position = master_clock();
    }

    // Cut down 
    int64_t prev = int64_t(clock);
    int64_t cur  = int64_t(new_position);

    if (prev != cur) {
        BTK_LOG("External clock to %ld\n", cur);
        if (audio_thread && video_thread) {
            auto audio = audio_thread->clock_time();
            auto video = video_thread->clock_time();
            BTK_LOG("Current Sync => A %lf V %lf A-V %lf\n", audio, video, audio - video);
            BTK_LOG("Current buffer %lfs\n", buffered());
        }
        if (!_position_changed.empty()) {
            defer_call(&Signal<void(double)>::emit, &_position_changed, double(cur));
        }
    }

    if (audio_thread && video_thread) {
        // BTK_LOG("A - V %lf\n", master_clock() - video_thread->clock_time());
    }

    clock = new_position;
}
void Demuxer::clock_pause(bool v) {
    if (v) {
        // Pause
        clock_update();
    }
    else {
        // Resume
        clock_start = av_gettime_relative() - clock * 1000000.0;
        clock_update();
    }
    clock_paused = v;
}
void Demuxer::handle_seek() {
    // This value tell video rerun
    bool set_video_running = false;
    bool set_audio_running = false;
    if (video_thread && !video_thread->is_paused()) {
        set_video_running = true;
        video_thread->pause(true);
    }
    if (audio_thread && !audio_thread->is_paused()) {
        set_audio_running = true;
        audio_thread->pause(true);
    }

    // Doing seek
    int ret;

    clock_pause(true);

    if (audio_thread) {
        audio_thread->packet_queue()->flush();
        audio_thread->packet_queue()->put(FlushPacket);

        int64_t pos = seek_pos / av_q2d(ctxt->streams[audio_stream]->time_base);
        ret = av_seek_frame(ctxt, audio_stream, pos, AVSEEK_FLAG_BACKWARD);
    }
    if (video_thread) {
        video_thread->packet_queue()->flush();
        video_thread->packet_queue()->put(FlushPacket);

        int64_t pos = seek_pos / av_q2d(ctxt->streams[video_stream]->time_base);
        ret = av_seek_frame(ctxt, video_stream, pos, AVSEEK_FLAG_BACKWARD);
    }

    clock = seek_pos;

    clock_pause(false);


    // Reset
    if (set_audio_running) {
        audio_thread->pause(false);
    }
    if (set_video_running) {
        video_thread->pause(false);
    }

    seek_req = false;
    seek_pos = 0.0;
}

bool Demuxer::seekable() {
    if (!ctxt) {
        return false;
    }
    if (ctxt->pb) {
        if (ctxt->pb->seekable == 0) {
            return false;
        }
    }
    return ctxt->iformat->read_seek || ctxt->iformat->read_seek2;
}
double Demuxer::duration() {
    if (!loaded) {
        return 0.0;
    }
    return ctxt->duration / double(AV_TIME_BASE);
}
double Demuxer::position() {
    return clock;
}
double Demuxer::master_clock() {
    // Currently use external clock
    if (sync_type == SyncType::AudioMaster && audio_thread) {
        return audio_thread->clock_time();
    }
    // Synctype == SyncType::ExternalMaster
    clock_update();
    return clock;
}
double Demuxer::buffered() {
    if (!ctxt) {
        return 0;
    }
    double seconds = std::numeric_limits<double>::max();
    int64_t duration = 0;
    if (audio_thread) {
        duration = audio_thread->packet_queue()->duration();
        seconds = min(seconds, duration * av_q2d(ctxt->streams[audio_stream]->time_base));
    }
    if (video_thread) {
        duration = video_thread->packet_queue()->duration();
        seconds = min(seconds, duration * av_q2d(ctxt->streams[video_stream]->time_base));
    }
    return seconds;
}
int Demuxer::error_code() {
    return av_code;
}
SyncType Demuxer::master_sync_type() {
    return sync_type;
}

void Demuxer::set_url(u8string_view u) {
    if (loaded) {
        unload();
    }
    url = u;
}
void Demuxer::set_position(double v) {
    if (state == State::Stopped) {
        return;
    }
    seek_pos = v;
    seek_req = true;
    demuxer_cond.notify_one();
}
void Demuxer::set_option(u8string_view key, u8string_view value) {
    int ret = av_dict_set(&options, u8string(key).c_str(), u8string(value).c_str(), 0);
    BTK_ASSERT(ret >= 0);
}
void Demuxer::set_state(State s) {
    if (state == s) {
        return;
    }
    state = s;
    if (_state_changed.empty()) {
        return;
    }
    if (ui_thread()) {
        _state_changed.emit(s);
    }
    else {
        defer_call(&Signal<void(State)>::emit, &_state_changed, s);
    }
}
void Demuxer::set_status(MediaStatus s) {
    if (status == s) {
        return;
    }
    status = s;
    if (_media_status_changed.empty()) {
        return;
    }
    if (ui_thread()) {
        _media_status_changed.emit(s);
    }
    else {
        defer_call(&Signal<void(MediaStatus)>::emit, &_media_status_changed, s);
    }
}


void Demuxer::pause(bool v) {
    if (v) {
        set_state(State::Paused);
    }
    else {
        set_state(State::Playing);
    }
    demuxer_cond.notify_one();
}

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)