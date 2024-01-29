#ifndef AUDIOMANAGER_H_
#define AUDIOMANAGER_H_

#include "portaudio.h"
#include "common/avdef.h"
#include "common/singleton.h"

class AudioParams
{
public:
    struct AudioData
    {
        int frame_index;
        int max_frame_index;
        void* data;
        int size;
    };
    AudioData audio_data;

    uint32_t sample_rate;
    AudioSampleFormat format;
    uint32_t channel_count;
    uint32_t frames_per_buffer;

    uint32_t frames(int32_t second) const { return sample_rate * second; }
    uint32_t samples_per_second() const { return channel_count * sample_rate; }
    int32_t bytes_samples_per_second() const { return sample_rate * bytes_sample_format(); }
    int32_t bytes_sample_format() const { return channel_count * bytes_sample_format_per_chl(); }

    PaSampleFormat sample_format() const
    {
        switch (format) {
        case kS8:
            return paInt8;
        case kU8:
            return paUInt8;
        case kS16:
            return paInt16;
        case kS24:
            return paInt24;
        case kS32:
            return paInt32;
        case kF32:
            return paFloat32;
        default:
            return 0;
        }
    }

    int32_t bytes_sample_format_per_chl() const
    {
        switch (format) {
        case kS8:
        case kU8:
            return 1;
        case kS16:
            return 2;
        case kS24:
            return 3;
        case kS32:
        case kF32:
            return 4;
        default:
            return 0;
        }
    }
};

class AudioManager
{
    SINGLETON_DECLARE(AudioManager)
public:
    AudioManager();
    ~AudioManager();

    bool Play(const char* filename);
    void Stop();

    bool StardRecording(bool async = true);

private:
    bool RecordingAsync();
    bool RecordingSync();

private:
    PaStream* stream_;
    AudioParams* audio_params_;
};

#endif
