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

class FFmpegWriter
{
public:
    explicit FFmpegWriter();
    ~FFmpegWriter();

    bool Open(const char* filename, AVStream* stream);
    bool Write(AVFrame* frame);
    void Close();

private:
    void FFmpegError(int error_code);
    bool AllocResource();
    void FreeResource();

private:
    AVFormatContext* fmt_ctx_;
    AVCodec* codec_;
    AVCodecContext* codec_ctx_;
    AVStream* new_stream_;

    AVPacket* packet_;

    bool header_written_;

    std::mutex mutex_;
};

#endif
