#pragma once

#include "build.hpp"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
}

BTK_PRIV_BEGIN

template <typename T>
class AVTraits;

template <>
class AVTraits<AVPacket> {
    public:
        void operator()(AVPacket *ptr) {
            av_packet_free(&ptr);
        }
};

template <>
class AVTraits<AVFrame> {
    public:
        void operator()(AVFrame *ptr) {
            av_frame_free(&ptr);
        }
};

template <>
class AVTraits<AVFormatContext> {
    public:
        void operator()(AVFormatContext *ptr) {
            avformat_close_input(&ptr);
        }
};

template <>
class AVTraits<AVCodecContext> {
    public:
        void operator()(AVCodecContext *ptr) {
            avcodec_free_context(&ptr);
        }
};

template <>
class AVTraits<SwrContext> {
    public:
        void operator()(SwrContext *ptr) {
            swr_free(&ptr);
        }
};

template <>
class AVTraits<SwsContext> {
    public:
        void operator()(SwsContext *ptr) {
            sws_freeContext(ptr);
        }
};

template <typename T>
class AVPtr : public std::unique_ptr<T, AVTraits<T>> {
    public:
        using std::unique_ptr<T, AVTraits<T>>::unique_ptr;
};


BTK_PRIV_END