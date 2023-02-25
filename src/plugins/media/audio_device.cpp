#include "build.hpp"

#include <Btk/plugins/media.hpp>
#include <mutex>

extern "C" {
#if defined(BTK_MINIAUDIO)
    #define  MA_IMPLEMENTATION
    #define  MA_USE_STDINT
    #define  MA_NO_DECODING
    #define  MA_NO_ENCODING
    #define  MA_NO_GENERATION
    #define  MA_API static

    #if !defined(NDEBUG)
        #define  MA_DEBUG_OUTPUT
    #endif

    #include <miniaudio.h>
#else
    #include <SDL2/SDL_audio.h>
    #include <SDL2/SDL.h>
#endif
}

BTK_NS_BEGIN

// Impl for audio device
class AudioDeviceImpl {
    public:
        AudioDevice::Routinue callback; //< Callback

        bool device_inited = false;

#if     defined(BTK_MINIAUDIO)
        ma_device      device;
#else
        SDL_AudioSpec     spec;
        SDL_AudioDeviceID device;
        std::vector<uint8_t> mixbuffer; //< Temp buffer for change the volume
#endif
        float volume = 1.0f;

        bool open(AudioSampleFormat format, int sample_rate, int channels);
        bool close();

        void run(void *buffer, uint32_t bytes);
        void pause(bool v);
        bool is_paused();
};

inline bool AudioDeviceImpl::open(AudioSampleFormat format, int sample_rate, int channels) {
    if (device_inited) {
        // Close previous
        close();
    }
#if defined(BTK_MINIAUDIO)
    // Convert to miniaudio format
    ma_format fmt;
    switch (format) {
        case AudioSampleFormat::Uint8 : fmt = ma_format_u8; break;
        case AudioSampleFormat::Sint16 : fmt = ma_format_s16; break;
        case AudioSampleFormat::Sint32 : fmt = ma_format_s32; break;
        case AudioSampleFormat::Float32 : fmt = ma_format_f32; break;
        default : return false;
    }

    ma_device_config conf = ma_device_config_init(ma_device_type_playback);
    conf.dataCallback = [](ma_device *dev, void *out, const void *in, ma_uint32 nframe) {
        static_cast<AudioDeviceImpl*>(dev->pUserData)->run(
            out,
            nframe * ma_get_bytes_per_frame(
                dev->playback.format,
                dev->playback.channels
            )
        );
    };
    conf.pUserData = this;
    conf.playback.format = fmt;
    conf.playback.channels = channels;
    conf.sampleRate = sample_rate;

    if (ma_device_init(nullptr, &conf, &device) != MA_SUCCESS) {
        return false;
    }
    device_inited = true;
    return true;
#else
    static std::once_flag once;
    std::call_once(once, SDL_Init, SDL_INIT_AUDIO);
    BTK_LOG("[SDL Audio backend] %s\n", SDL_GetCurrentAudioDriver());

    // Convert audio format to sdl format
    SDL_AudioFormat fmt;
    switch (format) {
        case AudioSampleFormat::Uint8 : fmt = AUDIO_U8; break;
        case AudioSampleFormat::Sint16 : fmt = AUDIO_S16; break;
        case AudioSampleFormat::Sint32 : fmt = AUDIO_S32; break;
        case AudioSampleFormat::Float32 : fmt = AUDIO_F32; break;
        default : return false;
    }

    // Make spec.
    spec = {};
    spec.callback = [](void *user, uint8_t *out, int len) {
        return static_cast<AudioDeviceImpl*>(user)->run(out, len);
    };
    spec.channels = channels;
    spec.format = fmt;
    spec.freq = sample_rate;
    // spec.samples = 1024;
    spec.userdata = this;

    device = SDL_OpenAudioDevice(nullptr, false, &spec, nullptr, 0);
    if (device == 0) {
        // Failed to open audio device
        BTK_LOG("[SDL Audio backend] Failed to open audio device : %s", SDL_GetError());
        return false;
    }
    device_inited = true;
    return true;
#endif

}
inline bool AudioDeviceImpl::close() {
    if (!device_inited) {
        // No inited, failed to close it
        return false;
    }
#if defined(BTK_MINIAUDIO)
    ma_device_uninit(&device);
#else
    SDL_CloseAudioDevice(device);
    device = 0;
#endif
    device_inited = false;
    return true;
}
inline void AudioDeviceImpl::pause(bool v) {
    if (!device_inited) {
        return;
    }
#if defined(BTK_MINIAUDIO)
    if (v) {
        ma_device_stop(&device);
    }
    else {
        ma_device_start(&device);
    }
#else
    SDL_PauseAudioDevice(device, v);
#endif
}
inline bool AudioDeviceImpl::is_paused() {
    if (!device_inited) {
        BTK_THROW(std::runtime_error("AudioDevice is not initialized"));
    }
#if defined(BTK_MINIAUDIO)
    return ma_device_get_state(&device) == ma_device_state_stopped;
#else
    return SDL_GetAudioDeviceStatus(device) == SDL_AUDIO_PAUSED;
#endif

}

// Main Callback
inline void AudioDeviceImpl::run(void *buffer, uint32_t bytes) {
    if (callback == nullptr) {
        // No callback, make it slience
        Btk_memset(buffer, 0, bytes);
        return;
    }

#if defined(BTK_MINIAUDIO)
    // Miniaudio 
    ma_device_set_master_volume(&device, volume);
    callback(buffer, bytes);
#else
    // SDL
    if (volume == 1.0f) {
        // No volume need be applied, 
        callback(buffer, bytes);
        return;
    }
    // Check mixbuffer
    if (mixbuffer.size() < bytes) {
        mixbuffer.resize(bytes);
    }

    // Fill current buffer
    callback(mixbuffer.data(), bytes);
    // Zero out buffer
    Btk_memset(buffer, 0, bytes);
    // Mix it
    SDL_MixAudioFormat(
        static_cast<uint8_t*>(buffer), 
        mixbuffer.data(), 
        spec.format, 
        bytes, 
        volume * 128 //< Convert normalized values [0, 1] => [0, 128]
    );
#endif
}


// Exposed interface for 
AudioDevice::AudioDevice() {
    priv = new AudioDeviceImpl;
}
AudioDevice::~AudioDevice() {
    priv->close();
    delete priv;
}

bool AudioDevice::open(AudioSampleFormat fmt, int sample_rate, int channels) {
    return priv->open(fmt, sample_rate, channels);
}
bool AudioDevice::close() {
    return priv->close();
}
void AudioDevice::pause(bool v) {
    return priv->pause(v);
}
bool AudioDevice::is_paused() {
    return priv->is_paused();
}
void AudioDevice::bind(const Routinue &cb) {
    priv->callback = cb;
}
void AudioDevice::set_volume(float volume) {
    priv->volume = clamp(volume, 0.0f, 1.0f);
}
float AudioDevice::volume() {
    return priv->volume;
}


BTK_NS_END