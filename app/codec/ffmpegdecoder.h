#ifndef FFmpegProcessor_H_
#define FFmpegProcessor_H_

#include <vector>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/fifo.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#include "common/media_info.h"
#include "util/decode_frame.h"

class Decoder
{
public:
    Decoder() = default;
    virtual ~Decoder() { thread_->join(); }

    void Init(AVCodecContext* codec_ctx, std::shared_ptr<PacketQueue> pkt_queue)
    {
        codec_ctx_ = codec_ctx;
        pkt_queue_ = pkt_queue;
        thread_ = std::make_unique<std::thread>(&Decoder::Decode, this);
    }

    virtual void Decode() = 0;

protected:
    std::unique_ptr<std::thread> thread_;
    std::shared_ptr<PacketQueue> pkt_queue_;
    AVCodecContext* codec_ctx_;
};

class VideoDecoder : public Decoder
{
public:
    VideoDecoder() = default;
    ~VideoDecoder() {}

    void Decode() override;
};

struct AVPacketInfo
{
    AVPacketInfo();

    int serial_num_;
    AVPacket* pkt_;
};

class PacketQueue
{
public:
    PacketQueue();

    void Push(AVPacket* pkt);
    void Pop(AVPacket* pkt);

private:
    int serial_num_;
    AVFifo* pkt_list;
    std::mutex mutex_;
};

class FrameQueue
{
public:
    FrameQueue();

    void Push(AVPacket* pkt);
    void Pop(AVPacket* pkt);

private:
    int serial_num_;
    AVFifo* pkt_list;
    std::mutex mutex_;
};

class FFmpegProcessor
{
public:
    FFmpegProcessor();
    ~FFmpegProcessor();

    void set_media(const MediaInfo& media) { media_ = media; }
    MediaInfo media() const { return media_; }

    bool Open();
    void Close();

    int GetPacket(AVPacket* pkt);
    DecodeFrame* GetFrame();

    const EncodeDataInfo* encode_data_info() const { return &encode_info_; }

    int fps() const { return fps_; }

    bool end() const { return eof_; }

    int64_t block_start_time() const { return block_start_time_; }
    int64_t block_timeout() const { return block_timeout_; }

private:
    bool OpenInputFormat();
    bool OpenStreamComponent(AVStream* stream, AVMediaType type);
    bool OpenDecoder();
    bool AllocFrame();
    void DoScalePrepare();
    void FillEncodeData();

    static AVPixelFormat GetDstPixFormat();
    static void AlignSize(int src_w, int src_h, int* dst_w, int* dst_h);

    bool InputFmt(std::string& url, AVInputFormat** fmt);
    AVDictionary* InputFmtOptions();

    void InitHwDecode(const AVCodec* codec);

    void ResizeDecodeFrame(int dst_w, int dst_h, int dst_pix_fmt);

    bool GpuDataToCpu(AVFrame* src, AVFrame* dst) const;

    bool Scale(AVFrame* src);

    void PushNullPacket(std::shared_ptr<PacketQueue> queue, AVPacket* pkt, int stream_index);

    // callback
    static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* fmt);

private:
    bool eof_;
    MediaInfo media_;

    AVFormatContext* fmt_ctx_;
    SwsContext* sws_ctx_;
    AVStream* video_stream_;
    AVStream* audio_stream_;
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

    int stream_indexs_[AVMEDIA_TYPE_NB];

    std::shared_ptr<PacketQueue> video_queue_;
    std::unique_ptr<Decoder> video_decoder_;
};

#endif
