#ifndef FFMPEGWRITER_H_
#define FFMPEGWRITER_H_

#include <atomic>
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
#include "util/decode_frame.h"

class FFmpegWriter
{
public:
    explicit FFmpegWriter();
    ~FFmpegWriter();

    void set_media(const MediaInfo& media);

    bool Open(const EncodeDataInfo& info);
    bool Write(const DecodeFrame& frame);
    void Close();

    bool opened() const { return opened_; };

    void Stop() { stop_ = true; }
    bool is_stop() const { return stop_; };

private:
    void FreeResource();

private:
    MediaInfo media_;
    std::atomic<bool> opened_;
    std::atomic<bool> stop_;

    AVFormatContext* fmt_ctx_;
    AVCodec* codec_;
    AVCodecContext* codec_ctx_;
    AVStream* video_stream_;

    AVFrame* frame_;
    AVPacket* packet_;

    bool header_written_;

    std::mutex mutex_;
    int64_t frame_index_;
};

#endif
