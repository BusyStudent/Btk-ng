#include "build.hpp"
#include "media_priv.hpp"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavutil/time.h>
}

BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

namespace {
    AVPixelFormat find_fmt_by_hw_type(AVHWDeviceType type) {
        switch (type) {
            case AV_HWDEVICE_TYPE_VAAPI:
                return AV_PIX_FMT_VAAPI;
            case AV_HWDEVICE_TYPE_DXVA2:
                return AV_PIX_FMT_DXVA2_VLD;
            case AV_HWDEVICE_TYPE_D3D11VA:
                return AV_PIX_FMT_D3D11;
            case AV_HWDEVICE_TYPE_VDPAU:
                return AV_PIX_FMT_VDPAU;
            case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
                return AV_PIX_FMT_VIDEOTOOLBOX;
            default : 
                return AV_PIX_FMT_NONE;
        }
    }
}


VideoThread::VideoThread(Demuxer *demuxer, AVStream *stream, AVCodecContext *ctxt) : demuxer(demuxer), stream(stream), ctxt(ctxt) {
    // TODO : Try hardware decode
    
    surface = demuxer->video_output();

    // Init convertion ctxt
    sws_ctxt.reset(
        sws_getContext(
            ctxt->width,
            ctxt->height,
            ctxt->pix_fmt,
            ctxt->width,
            ctxt->height,
            AV_PIX_FMT_RGBA,
            0,
            nullptr,
            nullptr,
            nullptr
        )
    );
    if (!sws_ctxt) {
        BTK_THROW(std::runtime_error("sws_getContext failed"));
    }
    size_t n     = av_image_get_buffer_size(AV_PIX_FMT_RGBA, ctxt->width, ctxt->height, 32);
    uint8_t *buf = static_cast<uint8_t*>(av_malloc(n));

    av_image_fill_arrays(
        dst_frame->data,
        dst_frame->linesize,
        buf,
        AV_PIX_FMT_RGBA,
        ctxt->width, ctxt->height,
        32
    );

    // Tell output we have started
    PixFormat fmt = PixFormat::RGBA32;
    surface->begin(&fmt);
    BTK_ASSERT(fmt == PixFormat::RGBA32);

    // All done, start video thread
    thrd = std::thread(&VideoThread::thread_main, this);
}
VideoThread::~VideoThread() {
    // Tell output we have doned
    manager.add_task(&AbstractVideoSurface::end, surface);

    pause(false);
    // Wait
    thrd.join();

    avcodec_free_context(&ctxt);
}

void VideoThread::thread_main() {
    SetThreadDescription("VideoThread");
    video_clock_start = av_gettime_relative();
    // Try get packet
    int ret;
    while (true) {
        if (paused) {
            std::unique_lock lock(cond_mtx);
            cond.wait(lock);
            continue;
        }
        AVPacket *packet = queue.get();
        if (packet == EofPacket) {
            // No more data
            nothing_to_do = true;
            continue;
        }
        if (packet == FlushPacket) {
            avcodec_flush_buffers(ctxt);
            manager.cancel_task(); // Cancel update task
            
            BTK_LOG(BTK_RED("[VideoThread] ") "Got flush\n");
            continue;
        }
        if (packet == StopPacket) {
            nothing_to_do = true;
            break; //< It tell us, it time to stop
        }
        if (packet == nullptr) {
            // No Data
            nothing_to_do = true;
            continue;
        }
        nothing_to_do = false;

        // Let's begin
        AVPtr<AVPacket> guard(packet);
        if (!video_decode_frame(packet)) {
            continue;
        }

        // Sync
        double current_frame_pts = src_frame->pts * av_q2d(stream->time_base);
        double master_clock = demuxer->master_clock();
        double diff = master_clock - video_clock;

        video_clock = current_frame_pts;

        if (diff < 0 && -diff < AVNoSyncThreshold) {
            // We are too fast
            auto delay = -diff;

            std::unique_lock lock(cond_mtx);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }
        else if (diff > 0.3) {
            // We are too slow, drop
            BTK_LOG(BTK_RED("[VideoThread] ") "A-V = %lf Too slow, drop\n", diff);
            continue;
        }
        else if (diff < AVSyncThreshold){
            // Sleep we we should
            double delay = 0;
            if (src_frame->pkt_duration != AV_NOPTS_VALUE) {
                delay = src_frame->pkt_duration * av_q2d(stream->time_base);
            }
            // BTK_LOG("duration %lf, diff %lf\n", delay, diff);
            // Sleep for it
            delay = min(delay, diff);

            std::unique_lock lock(cond_mtx);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }

        manager.add_task(&VideoThread::video_surface_ui_callback, this);

    }
}
void VideoThread::video_surface_ui_callback() {
    // EMM
    if (ctxt == nullptr) {
        return;
    }

    std::lock_guard locker(dst_frame_mtx);
    surface->write(PixFormat::RGBA32, dst_frame->data[0], dst_frame->linesize[0], ctxt->width, ctxt->height);
}
bool VideoThread::video_decode_frame(AVPacket *packet) {
    AVFrame *cvt_source = src_frame.get();
    int ret;
    ret = avcodec_send_packet(ctxt, packet);
    if (ret < 0) {
        return false;
    }
    ret = avcodec_receive_frame(ctxt, src_frame.get());
    if (ret < 0) {
        // Need more packet
        return false;
    }
    if (src_frame->format == hw_pix_fmt) {
        // Hardware decode
        ret = av_hwframe_transfer_data(sw_frame.get(), src_frame.get(), 0);
        if (ret < 0) {
            return false;
        }
        cvt_source = sw_frame.get();
    }
    // Convert it
    std::lock_guard locker(dst_frame_mtx);
    ret = sws_scale(
        sws_ctxt.get(),
        cvt_source->data,
        cvt_source->linesize,
        0,
        ctxt->height,
        dst_frame->data,
        dst_frame->linesize
    );
    if (ret < 0) {
        return false;
    }

    return true;
}
void VideoThread::pause(bool v) {
    paused = v;
    cond.notify_one();
}

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)