#ifndef FFMPEGDECODER_H_
#define FFMPEGDECODER_H_

#include <vector>

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

class FFmpegDecoder
{
public:
    explicit FFmpegDecoder();
    ~FFmpegDecoder();

    void set_media(const MediaInfo& media) { media_ = media; }
    MediaInfo media() { return media_; }

    bool Open();
    void Close();

    int GetPacket(AVPacket* pkt);
    DecodeFrame* GetFrame();

    bool is_end() { return end_; }
    AVStream* video_stream() { return video_stream_; }
    int fps() { return fps_; }

private:
    void FFmpegError(int error_code);
    bool AllocResource();
    void FreeResource();
    bool InitInputFmtParams(std::string& url, AVInputFormat** fmt);
    void InitDecodeParams();
    void InitHwDecode(const AVCodec* codec);
    bool GpuDataToCpu();

    // callback
    static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* fmt);

private:
    MediaInfo media_;

    AVFormatContext* fmt_ctx_;
    AVCodecContext* codec_ctx_;
    SwsContext* sws_ctx_;
    AVStream* video_stream_;
    AVPacket* packet_;
    AVFrame* frame_;

    AVFrame* hw_frame_;
    AVBufferRef* hw_dev_ctx_;
    std::vector<uint32_t> hw_devices_;

    DecodeFrame decode_frame_;
    uint8_t* data_[4];
    int linesize_[4];

    int64_t video_duration_;
    int fps_;
    int64_t frame_num_;
    bool end_;
};

#endif
