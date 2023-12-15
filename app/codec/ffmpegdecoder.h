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

    AVStream* video_stream() { return video_stream_; }
    int fps() { return fps_; }

    bool is_end() { return end_; }

    int64_t block_start_time() { return block_start_time_; }
    int64_t block_timeout() { return block_timeout_; }

private:
    bool OpenInputFormat();
    bool FindStream();
    bool OpenDecoder();
    bool AllocFrame();
    bool DoScalePrepare();

    bool InputFmt(std::string& url, AVInputFormat** fmt);
    AVDictionary* InputFmtOptions();

    void InitHwDecode(const AVCodec* codec);

    AVPixelFormat GetDstPixFormat();
    void ResizeDecodeFrame(int dst_w, int dst_h, int dst_pix_fmt);

    bool GpuDataToCpu(AVFrame* src, AVFrame* dst);

    bool Scale(AVFrame* src);

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

    bool hw_decode_;
    AVFrame* hw_frame_;
    AVBufferRef* hw_dev_ctx_;
    std::vector<uint32_t> hw_devices_;

    DecodeFrame decode_frame_;
    uint8_t* data_[4];
    int linesize_[4];

    int64_t block_start_time_;
    int64_t block_timeout_;

    int fps_;
    bool end_;
};

#endif
