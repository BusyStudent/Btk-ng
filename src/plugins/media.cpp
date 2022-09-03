#include "build.hpp"

#include <Btk/media.hpp>

// Import FFmpeg
extern "C" {
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
    #include <libavutil/avutil.h>
}

BTK_NS_BEGIN

namespace {
    template <typename T, auto Alloc, auto Free>
    class FFTraits {
        public:
            void operator()(T **v) {
                Free(v);
            }
        protected:
            static T *_Alloc() {
                return Alloc();
            }
    };
    template <typename T, typename Traits>
    class FFPtr : public std::unique_ptr<T, Traits>, public Traits {
        public:
            using std::unique_ptr<T, Traits>::unique_ptr;

            static FFPtr Alloc() {
                return Traits::_Alloc();
            }
    };

    template <typename T, auto Alloc, auto Free>
    using FFWrap = FFPtr<T, FFTraits<T, Alloc, Free>>;

    using AVPacketPtr = FFWrap<AVPacket, av_packet_alloc, av_packet_free>;
    using AVFramePtr  = FFWrap<AVFrame,  av_frame_alloc , av_frame_unref>;
}



BTK_NS_END