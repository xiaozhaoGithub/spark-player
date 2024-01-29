#include "audiomanager.h"

#include <algorithm>

#include "spdlog/spdlog.h"

AudioManager::AudioManager()
    : stream_(nullptr)
{
    PaError error_code = Pa_Initialize();
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to init port audio.");
    }

    audio_params_ = new AudioParams;
    audio_params_->sample_rate = 48000;
    audio_params_->format = kS16;
    audio_params_->channel_count = 2;
    audio_params_->frames_per_buffer = 1024;
}

AudioManager::~AudioManager()
{
    if (audio_params_->audio_data.data) {
        delete[] audio_params_->audio_data.data;
    }
    delete audio_params_;

    Pa_Terminate();
}

static int RecordCallback(const void* input, void* output, unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void* userData)
{
    int ret = paContinue;

    auto audio_params = static_cast<AudioParams*>(userData);

    int32_t left_frames =
        audio_params->audio_data.max_frame_index - audio_params->audio_data.frame_index;

    int32_t read_frames;
    if (left_frames < frameCount) {
        read_frames = left_frames;

        audio_params->audio_data.frame_index = 0;
        ret = paComplete;
    } else {
        read_frames = frameCount;
    }

    int32_t bytes = read_frames * audio_params->bytes_sample_format();
    int32_t offset = audio_params->audio_data.frame_index * audio_params->bytes_sample_format();

    if (!input) {
        memset(static_cast<char*>(audio_params->audio_data.data) + offset, 0, bytes);
    } else {
        memcpy(static_cast<char*>(audio_params->audio_data.data) + offset, input, bytes);
    }

    audio_params->audio_data.frame_index += read_frames;

    return ret;
}

static int PlayCallback(const void* input, void* output, unsigned long frameCount,
                        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
                        void* userData)
{
    int ret = paContinue;
    auto audio_params = static_cast<AudioParams*>(userData);

    int32_t left_frames =
        audio_params->audio_data.max_frame_index - audio_params->audio_data.frame_index;

    int32_t write_frames;
    if (left_frames < frameCount) {
        write_frames = left_frames;
        ret = paComplete;
    } else {
        write_frames = frameCount;
    }

    int32_t write_bytes = write_frames * audio_params->bytes_sample_format();
    int32_t offset = audio_params->audio_data.frame_index * audio_params->bytes_sample_format();
    memcpy(output, static_cast<char*>(audio_params->audio_data.data) + offset, write_bytes);

    if (ret == paComplete) {
        int32_t left_bytes = (frameCount - write_frames) * audio_params->bytes_sample_format();
        memset(static_cast<char*>(output) + write_bytes, 0, left_bytes);
    }

    audio_params->audio_data.frame_index += write_frames;

    return ret;
}

bool AudioManager::Play(const char* filename)
{
    Stop();

    int second = 5;
    audio_params_->audio_data.frame_index = 0;
    audio_params_->audio_data.max_frame_index = audio_params_->frames(second);
    // int64_t buf_size = second * audio_params_->bytes_samples_per_second();
    // audio_params_->audio_data.data = realloc(audio_params_->audio_data.data, buf_size);

    PaDeviceIndex output_dev_index = Pa_GetDefaultOutputDevice();
    if (output_dev_index == paNoDevice) {
        SPDLOG_ERROR("Failed to get default output device.");
        return false;
    }

    PaStreamParameters out_param;
    out_param.device = output_dev_index;
    out_param.channelCount = audio_params_->channel_count;
    out_param.sampleFormat = audio_params_->sample_format();
    out_param.suggestedLatency = Pa_GetDeviceInfo(out_param.device)->defaultLowOutputLatency;
    out_param.hostApiSpecificStreamInfo = nullptr;

    PaError error_code =
        Pa_OpenStream(&stream_, nullptr, &out_param, audio_params_->sample_rate,
                      audio_params_->frames_per_buffer, paClipOff, PlayCallback, audio_params_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to open pa stream.");
        return false;
    }

    SPDLOG_INFO("Begin playback audio.");
    error_code = Pa_StartStream(stream_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to start pa stream.");
        return false;
    }

    while ((error_code = Pa_IsStreamActive(stream_)) == 1) {
        Pa_Sleep(1000);
        printf("index = %d\n", audio_params_->audio_data.frame_index);
        fflush(stdout);
    }

    Pa_CloseStream(stream_);

    return true;
}

void AudioManager::Stop()
{
    if (Pa_IsStreamActive(stream_)) {
        Pa_CloseStream(stream_);
    }
}

bool AudioManager::StardRecording(bool async)
{
    bool ret;
    if (async) {
        ret = RecordingAsync();
    } else {
        ret = RecordingSync();
    }

    return ret;
}

bool AudioManager::RecordingAsync()
{
    int second = 5;
    audio_params_->audio_data.frame_index = 0;
    audio_params_->audio_data.max_frame_index = audio_params_->frames(second);

    audio_params_->audio_data.size = second * audio_params_->bytes_samples_per_second();
    audio_params_->audio_data.data = new int8_t[audio_params_->audio_data.size];
    memset(audio_params_->audio_data.data, 0, sizeof(int8_t) * audio_params_->audio_data.size);

    PaDeviceIndex input_dev_index = Pa_GetDefaultInputDevice();
    if (input_dev_index == paNoDevice) {
        SPDLOG_ERROR("Failed to get default input device.");
        return false;
    }

    PaStreamParameters in_param;
    in_param.device = input_dev_index;
    in_param.channelCount = audio_params_->channel_count;
    in_param.sampleFormat = audio_params_->sample_format();
    in_param.suggestedLatency = Pa_GetDeviceInfo(in_param.device)->defaultLowInputLatency;
    in_param.hostApiSpecificStreamInfo = nullptr;

    PaError error_code =
        Pa_OpenStream(&stream_, &in_param, nullptr, audio_params_->sample_rate,
                      audio_params_->frames_per_buffer, paClipOff, RecordCallback, audio_params_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to open pa stream.");
        return false;
    }

    error_code = Pa_StartStream(stream_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to start pa stream.");
        return false;
    }

    while ((error_code = Pa_IsStreamActive(stream_)) == 1) {
        Pa_Sleep(1000);
        printf("index = %d\n", audio_params_->audio_data.frame_index);
        fflush(stdout);
    }

    FILE* out_fp = fopen("recorded.pcm", "wb");
    if (!out_fp) {
        return false;
    }
    SPDLOG_INFO("Start to write recorded raw data.");
    fwrite(audio_params_->audio_data.data, 1, audio_params_->audio_data.size, out_fp);
    fclose(out_fp);

    Pa_CloseStream(stream_);

    Play("");

    return true;
}

bool AudioManager::RecordingSync()
{
    PaDeviceIndex input_dev_index = Pa_GetDefaultInputDevice();
    if (input_dev_index == paNoDevice) {
        SPDLOG_ERROR("Failed to get default input device.");
        return false;
    }

    PaStreamParameters in_param;
    in_param.device = input_dev_index;
    in_param.channelCount = audio_params_->channel_count;
    in_param.sampleFormat = audio_params_->sample_format();
    in_param.suggestedLatency = Pa_GetDeviceInfo(in_param.device)->defaultLowInputLatency;
    in_param.hostApiSpecificStreamInfo = nullptr;

    // test sync
    PaError error_code = Pa_OpenStream(&stream_, &in_param, nullptr, audio_params_->sample_rate,
                                       audio_params_->frames_per_buffer, paClipOff, nullptr, nullptr);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to open pa stream.");
        return false;
    }

    error_code = Pa_StartStream(stream_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to start pa stream.");
        return false;
    }

    int second = 5;
    uint32_t frames = audio_params_->frames(second);

    int64_t buf_size = second * audio_params_->bytes_samples_per_second();
    int8_t* buf = new int8_t[buf_size];
    memset(buf, 0, sizeof(int8_t) * buf_size);

    Pa_ReadStream(stream_, buf, frames); // blocking

    Pa_CloseStream(stream_);


    SPDLOG_INFO("Begin playback audio.");
    PaDeviceIndex output_dev_index = Pa_GetDefaultOutputDevice();
    if (output_dev_index == paNoDevice) {
        SPDLOG_ERROR("Failed to get default output device.");
        return false;
    }

    PaStreamParameters out_param;
    out_param.device = output_dev_index;
    out_param.channelCount = audio_params_->channel_count;
    out_param.sampleFormat = audio_params_->sample_format();
    out_param.suggestedLatency = Pa_GetDeviceInfo(out_param.device)->defaultLowOutputLatency;
    out_param.hostApiSpecificStreamInfo = nullptr;

    // test sync
    error_code = Pa_OpenStream(&stream_, nullptr, &out_param, audio_params_->sample_rate,
                               audio_params_->frames_per_buffer, paClipOff, nullptr, nullptr);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to open pa stream.");
        return false;
    }

    error_code = Pa_StartStream(stream_);
    if (error_code != paNoError) {
        SPDLOG_ERROR("Failed to start pa stream.");
        return false;
    }

    Pa_WriteStream(stream_, buf, frames);
    Pa_CloseStream(stream_);

    delete[] buf;

    return true;
}
