#ifndef FFMPEGWRITER_H_
#define FFMPEGWRITER_H_

#include <mutex>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#include "common/media_info.h"

class FFmpegWriter
{
public:
    explicit FFmpegWriter();
    ~FFmpegWriter();

    void set_media(const MediaInfo& media);

    bool Open(AVStream* stream);
    bool Write(AVFrame* frame);
    void Close();

private:
    void FFmpegError(int error_code);
    bool AllocResource();
    void FreeResource();

private:
    std::unique_ptr<MediaInfo> media_;

    AVFormatContext* fmt_ctx_;
    AVCodec* codec_;
    AVCodecContext* codec_ctx_;
    AVStream* new_stream_;

    AVPacket* packet_;

    bool header_written_;

    std::mutex mutex_;
    int64_t frame_index_;
};

#endif
