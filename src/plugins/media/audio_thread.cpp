#include "build.hpp"
#include "media_priv.hpp"

#define SAMPLE_CORRECTION_PERCENT_MAX 10

extern "C" {
    #include <libavutil/opt.h>
}

BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

AudioThread::AudioThread(Demuxer *demuxer, AVStream *stream, AVCodecContext *ctxt) : demuxer(demuxer), stream(stream), ctxt(ctxt) {
    device = demuxer->audio_output();
    // Try Open Audio
    device->bind([this](void *v, uint32_t len) { audio_callback(v, len); });

    // Open it
    device->open(
        AudioSampleFormat::Float32,
        ctxt->sample_rate,
        ctxt->channels
    );
    device->pause(0);
}
AudioThread::~AudioThread() {
    device->close();

    avcodec_free_context(&ctxt);
}

void AudioThread::audio_callback(void *_dst, int len) {
    uint8_t *dst = static_cast<uint8_t*>(_dst);
    while (len > 0) {
        if (buffer_index >= buffer.size()) {
            // Run out of buffer
            int audio_size = audio_decode_frame();
            if (audio_size < 0) {
                // No audio
                break;
            }
        }

        // Write buffer
        int left = buffer.size() - buffer_index;
        left = min(len, left);

        Btk_memcpy(dst, buffer.data() + buffer_index, left);
        
        dst += left;
        
        len -= left;
        buffer_index += left;
    }

    // Make slience
    if (len) {
        Btk_memset(dst, 0, len);
    }
}
int AudioThread::audio_decode_frame() {
    // Try get packet
    int ret;
    while (true) {
        AVPacket *packet = queue.get(false);
        if (packet == EofPacket) {
            // No more data
            nothing_to_do = true;
            return -1;
        }
        if (packet == FlushPacket) {
            avcodec_flush_buffers(ctxt);
            swr_ctxt.reset();

            BTK_LOG("AudioThread Got flush\n");
            continue;
        }
        if (packet == StopPacket) {
            nothing_to_do = true;
            return -1;
        }
        if (packet == nullptr) {
            // No Data
            nothing_to_do = true;
            return -1;
        }
        nothing_to_do = false;

        // Normal data
        AVPtr<AVPacket> guard(packet);

        ret = avcodec_send_packet(ctxt, packet);
        if (ret < 0) {
            // Too much
            if (ret != AVERROR(EAGAIN)) {
                return -1;
            }
        }
        ret = avcodec_receive_frame(ctxt, frame.get());
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                // We need more
                continue;
            }
            return -1; // Err
        }

        // Update audio clock 
        if (frame->pts == AV_NOPTS_VALUE) {
            audio_clock = 0.0;
        }
        else {
            audio_clock = frame->pts * av_q2d(stream->time_base);
            // BTK_LOG("Audio clock %lf\n", audio_clock);
        }

        // Got it
        int size = audio_resample(audio_sync(frame->nb_samples));
        if (size < 0) {
            return size;
        }

        return size;
    }
}
int AudioThread::audio_resample(int wanted_samples) {
    // Get frame samples buffer size
    int data_size = av_samples_get_buffer_size(nullptr, ctxt->channels, frame->nb_samples, AVSampleFormat(frame->format), 1);

    auto out_sample_rate = ctxt->sample_rate;
    auto in_sample_rate = frame->sample_rate;

    if (!swr_ctxt) {
        // init swr context
        int64_t in_channel_layout = (ctxt->channels ==
                            av_get_channel_layout_nb_channels(ctxt->channel_layout)) ?
                            ctxt->channel_layout :
                            av_get_default_channel_layout(ctxt->channels);
        auto    out_chanel_layout = in_channel_layout;

        BTK_ASSERT(frame->format == ctxt->sample_fmt);

        swr_ctxt.reset(
            swr_alloc_set_opts(
                nullptr,
                out_chanel_layout,
                AV_SAMPLE_FMT_FLT,
                out_sample_rate,
                in_channel_layout,
                ctxt->sample_fmt,
                in_sample_rate,
                0,
                nullptr
            )
        );
        if (!swr_ctxt || swr_init(swr_ctxt.get()) < 0) {
            BTK_LOG("swr_alloc_set_opts() failed\n");
            return -1;
        }
    }
    if (swr_ctxt) {
        // Convert
        uint8_t *buffer_data;

        const uint8_t **in = (const uint8_t**) frame->data;
        uint8_t **out = &buffer_data;

        int out_count = int64_t(wanted_samples) * out_sample_rate / in_sample_rate;
        int out_size = av_samples_get_buffer_size(nullptr, ctxt->channels, out_count, ctxt->sample_fmt, 1);
        if (out_size < 0) {
            BTK_LOG("av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_samples != frame->nb_samples) {
            if (swr_set_compensation(swr_ctxt.get(), (wanted_samples - frame->nb_samples) * out_sample_rate / frame->sample_rate,
                                                    wanted_samples * out_sample_rate / frame->sample_rate) < 0) {
                BTK_LOG("swr_set_compensation failed\n");
                return -1;
            }
        }
        buffer.resize(out_size);
        buffer_data = buffer.data();

        int len2 = swr_convert(swr_ctxt.get(), out, out_count, in, frame->nb_samples);
        if (len2 < 0) {
            BTK_LOG("swr_convert failed\n");
            return -1;
        }
        if (len2 == out_count) {
            // BTK_LOG("audio buffer is probably too small\n");
        }
        int resampled_data_size = len2 * ctxt->channels * av_get_bytes_per_sample(ctxt->sample_fmt);
        buffer.resize(resampled_data_size);
        buffer_index = 0;
        return resampled_data_size;
    }
    else {
        // Opps
        BTK_LOG("swr_alloc_set_opts() failed\n");
        return -1;
    }
}
// int AudioThread::audio_resample(int out_samples) {
//     // Setting up 
//     int64_t in_channal_layout = (ctxt->channels ==
//                         av_get_channel_layout_nb_channels(ctxt->channel_layout)) ?
//                         ctxt->channel_layout :
//                         av_get_default_channel_layout(ctxt->channels);
    

//     auto    out_channal_layout = in_channal_layout;
//     auto    out_sample_fmt     = AV_SAMPLE_FMT_FLT;
//     auto    out_channals       = ctxt->channels;

//     SwrContext *cvt = swr_ctxt.get();
//     if (!cvt) {
//         swr_ctxt.reset(swr_alloc());
//         cvt = swr_ctxt.get();
//         // Set convert args
//         av_opt_set_int(cvt, "in_channel_layout", in_channal_layout, 0);
//         av_opt_set_int(cvt, "in_sample_rate", ctxt->sample_rate, 0);
//         av_opt_set_sample_fmt(cvt, "in_sample_fmt", ctxt->sample_fmt, 0);

//         av_opt_set_int(cvt, "out_channel_layout", out_channal_layout, 0);
//         av_opt_set_int(cvt, "out_sample_rate", ctxt->sample_rate, 0);
//         av_opt_set_sample_fmt(cvt, "out_sample_fmt", out_sample_fmt, 0);

//         if (swr_init(cvt) < 0) {
//             return -1;
//         }
//     }

//     auto nsamples = av_rescale_rnd(
//         // swr_get_delay(cvt, ctxt->sample_rate) + frame->nb_samples,
//         out_samples,
//         ctxt->sample_rate,
//         ctxt->sample_rate,
//         AV_ROUND_UP
//     );
//     if (nsamples < 0) {
//         return -1;
//     }
//     if (nsamples != frame->nb_samples) {
//         // Set compress
//         int ret = swr_set_compensation(
//             cvt,
//             (out_samples - frame->nb_samples) * ctxt->sample_rate / frame->sample_rate,
//             out_samples * ctxt->sample_rate / frame->sample_rate
//         );
//         if (ret < 0) {
//             BTK_LOG("failed to swr_set_compensation %d, %s\n", ret, "");
//         }
//     }

//     int out_linesize;
//     auto buffersize = av_samples_get_buffer_size(
//         &out_linesize,
//         out_channals,
//         nsamples,
//         out_sample_fmt,
//         1
//     );
//     if (buffersize < 0)  {
//         return -1;
//     }

//     // Alloc
//     buffer.resize(buffersize);
//     buffer_index = 0;

//     uint8_t *addr = buffer.data();
//     auto ret = swr_convert(
//         cvt,
//         &addr,
//         nsamples,
//         (const uint8_t**)frame->data,
//         frame->nb_samples
//     );
//     if (ret < 0) {
//         return -1;
//     }

//     // Calc how many resampled
//     buffersize = av_samples_get_buffer_size(
//         &out_linesize,
//         out_channals,
//         ret,
//         out_sample_fmt,
//         1
//     );
//     if (buffersize < 0)  {
//         return -1;
//     }
//     if (buffersize != buffer.size()) {
//         BTK_LOG("Buffersize != buffer.size()\n in %d, out %d wanted %d\n", frame->nb_samples, ret, nsamples);
//         buffer.resize(buffersize);
//     }

//     return buffersize;
// }
int AudioThread::audio_sync(int nb_samples) {
    if (demuxer->master_sync_type() == SyncType::AudioMaster) {
        // Sync by self
        return nb_samples;
    }

    // Calc current clock value
    auto wanted_samples = nb_samples;

    auto clock = audio_clock;
    auto refclock = demuxer->master_clock();
    auto diff = clock - refclock;

    if (diff < AVNoSyncThreshold) {
        // accumulate
        audio_diff_cum = diff + audio_diff_avg_coef * audio_diff_cum;
        if (audio_diff_avg_count < AudioDiffAvgNB) {
            // No need
            audio_diff_avg_count ++;
            return wanted_samples;
        }

        auto avg_diff = audio_diff_cum * (1.0 - audio_diff_avg_coef);

        if (std::abs(avg_diff) >= audio_diff_threshold) {

            wanted_samples = nb_samples + int(diff * ctxt->sample_rate);
            int min_size = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
            int max_size = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));

            wanted_samples = clamp(wanted_samples, min_size, max_size);
        }
    }
    else {
        BTK_LOG("Diff is too big\n");
        // Diif is too big
        audio_diff_avg_count = 0;
        audio_diff_cum = 0;
    }

    return wanted_samples;
}

BTK_NS_END2(BTN_NAMESPACE::FFmpeg)