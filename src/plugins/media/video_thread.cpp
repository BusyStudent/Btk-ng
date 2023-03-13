#include "build.hpp"
#include "media_priv.hpp"

extern "C" {
    #include <libavutil/hwcontext.h>
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
    video_try_hw();
    
    surface = demuxer->video_output();

    // Prepare convertion buffer
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
    pause(false);
    // Wait
    thrd.join();

    avcodec_free_context(&ctxt);
}
void VideoThread::video_try_hw() {
    static AVPixelFormat static_hw_fmt;
    AVHWDeviceType hwtype;

    // Try create a context with hardware
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVPtr<AVCodecContext> hw_cctxt(avcodec_alloc_context3(codec));
    if (!hw_cctxt) {
        return;
    }
    if (avcodec_parameters_to_context(hw_cctxt.get(), stream->codecpar) < 0) {
        return;
    }

    // Get Configuaration
    const AVCodecHWConfig *conf = nullptr;
    for (int i = 0; ; i++) {
        conf = avcodec_get_hw_config(codec, i);
        if (!conf) {
            return;
        }

        if (conf->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
            // Got
            hw_pix_fmt = conf->pix_fmt;
            static_hw_fmt = conf->pix_fmt;
            hwtype = conf->device_type;
            break;
        }
    }

    // Override HW get format
    hw_cctxt->get_format = [](struct AVCodecContext *s, const enum AVPixelFormat * fmt) {
        return static_hw_fmt;
    };

    // Try init hw
    AVBufferRef *hw_dev_ctxt = nullptr;
    if (av_hwdevice_ctx_create(&hw_dev_ctxt, hwtype, nullptr, nullptr, 0) < 0) {
        // Failed
        return;
    }
    hw_cctxt->hw_device_ctx = av_buffer_ref(hw_dev_ctxt);

    // Init codec
    if (avcodec_open2(hw_cctxt.get(), codec, nullptr) < 0) {
        return;
    }

    // Done
    avcodec_free_context(&ctxt);
    ctxt = hw_cctxt.release();

    BTK_LOG(BTK_RED("[VideoThread] ") "Hardware decoder from %s\n", av_hwdevice_get_type_name(hwtype));
}
void VideoThread::thread_main() {
    SetThreadDescription("VideoThread");
    video_clock_start = av_gettime_relative();
    sws_scale_duration = 0.0;
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
        AVFrame *frame;
        if (!video_decode_frame(packet, &frame)) {
            continue;
        }

        // TODO : Add sws_scale_duration to adjust the time
        // Sync
        double current_frame_pts = src_frame->pts * av_q2d(stream->time_base);
        double master_clock = demuxer->master_clock();
        double diff = master_clock - video_clock - sws_scale_duration;

        video_clock = current_frame_pts;

        if (diff < 0 && -diff < AVNoSyncThreshold) {
            // We are too fast
            auto delay = -diff;

            std::unique_lock lock(cond_mtx);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }
        else if (diff > 0.3) {
            // We are too slow, drop
            BTK_LOG(BTK_RED("[VideoThread] ") "A-V = %lf Too slow, drop sws_duration = %lf\n", diff, sws_scale_duration);
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

        video_write_frame(frame);
    }
    // Tell output we have doned
    surface->end();
}
void VideoThread::video_surface_ui_callback() {
    // EMM
    if (ctxt == nullptr) {
        return;
    }

    std::lock_guard locker(dst_frame_mtx);
    surface->write(PixFormat::RGBA32, dst_frame->data[0], dst_frame->linesize[0], ctxt->width, ctxt->height);
}
bool VideoThread::video_decode_frame(AVPacket *packet, AVFrame **ret_frame) {
    AVFrame *cvt_source = src_frame.get();
    int ret;
    ret = avcodec_send_packet(ctxt, packet);
    if (ret < 0) {
        BTK_LOG(BTK_RED("[VideoThread] ") "avcodec_send_packet failed!!!\n");
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

    *ret_frame = cvt_source;
    return true;
}
void VideoThread::video_write_frame(AVFrame *source) {
    
    // Lazy eval beacuse of the hardware access
    if (!sws_ctxt) {
        sws_ctxt.reset(
            sws_getContext(
                ctxt->width,
                ctxt->height,
                AVPixelFormat(source->format),
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
            BTK_LOG(BTK_RED("[VideoThread] ") "sws_getContext failed!!!\n");
            return;
        }
    }

    // Convert it
    std::lock_guard locker(dst_frame_mtx);
    
    int64_t sws_begin_time = av_gettime_relative();
    int ret = sws_scale(
        sws_ctxt.get(),
        source->data,
        source->linesize,
        0,
        ctxt->height,
        dst_frame->data,
        dst_frame->linesize
    );

    sws_scale_duration = (av_gettime_relative() - sws_begin_time) / 1000000.0;

    if (ret < 0) {
        BTK_LOG(BTK_RED("[VideoThread] ") "sws_scale failed %d!!!\n", ret);
        return;
    }

    manager.add_task(&VideoThread::video_surface_ui_callback, this);
    return;
}
void VideoThread::pause(bool v) {
    paused = v;
    cond.notify_one();
}

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)