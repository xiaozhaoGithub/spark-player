#ifndef FFMPEGDECODER_H_
#define FFMPEGDECODER_H_

#include <QImage>
#include <QObject>
#include <QSize>
#include <QTime>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class FFmpegDecoder : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegDecoder(QObject* parent = nullptr);
    ~FFmpegDecoder();

    bool Open(const char* filename);
    void Close();

    int GetPacket(AVPacket* pkt);
    QImage GetFrame();

    inline bool is_end();
    inline int64_t pts();

private:
    void FFmpegError(int error_code);

private:
    AVFormatContext* fmt_ctx_;
    AVCodecContext* codec_ctx_;
    SwsContext* sws_ctx_;
    AVStream* stream_;
    AVPacket* packet_;
    AVFrame* frame_;

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

#endif
