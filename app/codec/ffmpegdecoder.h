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
    FFmpegDecoder();
    ~FFmpegDecoder();

    void set_media(const MediaInfo& media) { media_ = media; }
    MediaInfo media() const { return media_; }

    bool Open();
    void Close();

    int GetPacket(AVPacket* pkt);
    DecodeFrame* GetFrame();

    const EncodeDataInfo* encode_data_info() const { return &encode_info_; }

    int fps() const { return fps_; }

    bool end() const { return end_; }

    int64_t block_start_time() const { return block_start_time_; }
    int64_t block_timeout() const { return block_timeout_; }

private:
    bool OpenInputFormat();
    bool FindStream();
    bool OpenDecoder();
    bool AllocFrame();
    void DoScalePrepare();
    void FillEncodeData();

    static AVPixelFormat GetDstPixFormat();

    bool InputFmt(std::string& url, AVInputFormat** fmt);
    AVDictionary* InputFmtOptions();

    void InitHwDecode(const AVCodec* codec);

    void ResizeDecodeFrame(int dst_w, int dst_h, int dst_pix_fmt);

    bool GpuDataToCpu(AVFrame* src, AVFrame* dst) const;

    void AlignSize(int src_w, int src_h, int* dst_w, int* dst_h);
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

    EncodeDataInfo encode_info_;

    int64_t block_start_time_;
    int64_t block_timeout_;

    int fps_;
    bool end_;
};

#endif
