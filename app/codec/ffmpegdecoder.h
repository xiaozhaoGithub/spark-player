#ifndef FFMPEGDECODER_H_
#define FFMPEGDECODER_H_

#include <QImage>
#include <QObject>
#include <QSize>
#include <QTime>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#include "common/media_info.h"

class FFmpegDecoder : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegDecoder(QObject* parent = nullptr);
    ~FFmpegDecoder();

    void set_media(const MediaInfo& media);
    MediaInfo* media();

    bool Open();
    void Close();

    int GetPacket(AVPacket* pkt);
    AVFrame* GetFrame();

    inline bool is_end();
    inline int64_t pts();
    inline AVStream* stream();

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
    std::unique_ptr<MediaInfo> media_;

    AVFormatContext* fmt_ctx_;
    AVCodecContext* codec_ctx_;
    SwsContext* sws_ctx_;
    AVStream* stream_;
    AVPacket* packet_;
    AVFrame* frame_;

    AVFrame* hw_frame_;
    AVBufferRef* hw_dev_ctx_;
    QVector<uint32_t> hw_devices;

    int64_t video_duration_;
    double frame_rate_;
    int64_t frame_num_;
    QSize video_resolution_;
    uint8_t* image_buf_;
    bool end_;
    int64_t pts_;
};

inline bool FFmpegDecoder::is_end()
{
    return end_;
}

inline int64_t FFmpegDecoder::pts()
{
    return pts_;
}

inline AVStream* FFmpegDecoder::stream()
{
    return stream_;
}

#endif
