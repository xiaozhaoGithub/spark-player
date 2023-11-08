#include "ffmpegdecoder.h"

#include <QImage>

#include "common/singleton.h"
#include "config/config.h"
#include "spdlog/spdlog.h"
#include "widget/video_display/video_display_widget.h"

FFmpegDecoder::FFmpegDecoder(QObject* parent)
    : QObject(parent)
    , fmt_ctx_(nullptr)
    , codec_ctx_(nullptr)
    , sws_ctx_(nullptr)
    , stream_(nullptr)
    , packet_(nullptr)
    , frame_(nullptr)
    , image_buf_(nullptr)
    , soft_decode_(true)
    , end_(true)
    , pts_(0)
{
    fmt_ctx_ = avformat_alloc_context();

    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
        hw_devices.push_back(type);
    }
}

FFmpegDecoder::~FFmpegDecoder()
{
    Close();
}

bool FFmpegDecoder::Open(const char* filename)
{
    AVDictionary* dict = nullptr;
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);
    av_dict_set(&dict, "max_delay", "3", 0);
    av_dict_set(&dict, "timeout", "1000000", 0);

    int error_code = avformat_open_input(&fmt_ctx_, filename, nullptr, &dict);
    if (error_code != 0) {
        SPDLOG_ERROR("Failed to open input stream.");
        return false;
    }

    if (dict) {
        av_dict_free(&dict);
    }

    SPDLOG_INFO("--------------- File Information ----------------.");
    av_dump_format(fmt_ctx_, 0, filename, 0);
    SPDLOG_INFO("-------------------------------------------------");

    error_code = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to find stream information.");
        return false;
    }

    video_duration_ = fmt_ctx_->duration / (AV_TIME_BASE / 1000);
    QString duration = QTime::fromMSecsSinceStartOfDay(video_duration_).toString("HH:mm:ss zzz");
    SPDLOG_INFO("Video total time:{0}ms, [{1}].", video_duration_, duration.toStdString());

    int video_index = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_index < 0) {
        SPDLOG_ERROR("Failed to find a video stream.");
        return false;
    }

    stream_ = fmt_ctx_->streams[video_index];
    video_resolution_.setWidth(stream_->codecpar->width);
    video_resolution_.setHeight(stream_->codecpar->height);

    frame_rate_ = av_q2d(stream_->avg_frame_rate);
    frame_num_ = stream_->nb_frames;

    const AVCodec* codec = avcodec_find_decoder(stream_->codecpar->codec_id);
    if (!codec) {
        SPDLOG_ERROR("Failed to find Codec.");
        return false;
    }
    SPDLOG_INFO("resolution: [w:{0}, h:{1}] frame rate:{2} total frames:{3} codec name:{4}",
                video_resolution_.width(), video_resolution_.height(), frame_rate_, frame_num_,
                codec->name);

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        SPDLOG_ERROR("Failed to create video decoder context.");
        return false;
    }

    error_code = avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to fill the codec context.");
        return false;
    }

    error_code = av_dict_set(&dict, "threads", "auto", 0);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to set codec options, performance may suffer.");
        return false;
    }

    bool enable_hardware_dc =
        Singleton<Config>::Instance()->AppConfigData("video_param", "enable_hardware_dc", false).toBool();
    if (enable_hardware_dc) {
        InitHardWareDc();
    }

    error_code = avcodec_open2(codec_ctx_, codec, &dict);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to open codec.");
        return false;
    }

    if (dict) {
        av_dict_free(&dict);
    }

    packet_ = av_packet_alloc();
    if (!packet_) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    frame_ = av_frame_alloc();
    if (!frame_) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }

    int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_resolution_.width(),
                                        video_resolution_.height(), 4);

    image_buf_ = new uint8_t[size];
    end_ = false;

    return true;
}

void FFmpegDecoder::Close()
{
    if (fmt_ctx_) {
        avformat_close_input(&fmt_ctx_);
    }

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }

    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }

    if (packet_) {
        av_packet_free(&packet_);
    }

    if (frame_) {
        av_frame_free(&frame_);
    }

    if (image_buf_) {
        delete[] image_buf_;
        image_buf_ = nullptr;
    }
}

int FFmpegDecoder::GetPacket(AVPacket* pkt)
{
    int ret;

    do {
        av_packet_unref(pkt);
        ret = av_read_frame(fmt_ctx_, pkt);
    } while (pkt->stream_index != stream_->index && ret >= 0);

    return ret;
}

QImage FFmpegDecoder::GetFrame()
{
    if (!fmt_ctx_)
        return QImage();

    int ret = GetPacket(packet_);
    if (ret == 0) {
        if (packet_->stream_index == stream_->index) {
            packet_->pts = qRound64(packet_->pts * (1000 * av_q2d(stream_->time_base)));
            packet_->dts = qRound64(packet_->dts * (1000 * av_q2d(stream_->time_base)));

            int ret = avcodec_send_packet(codec_ctx_, packet_);
            if (ret != 0) {
                FFmpegError(ret);
            }
        }
    } else if (ret == AVERROR_EOF) {
        avcodec_send_packet(codec_ctx_, nullptr); // Send null packet to represent end.
    } else {
        FFmpegError(ret);
    }
    av_packet_unref(packet_);

    int error_code = avcodec_receive_frame(codec_ctx_, frame_);
    if (error_code != 0) {
        av_frame_unref(frame_);
        FFmpegError(error_code);

        if (ret < 0) {
            end_ = true;
        }
        return QImage();
    }
    pts_ = frame_->pts;

    if (!sws_ctx_) {
        sws_ctx_ = sws_getContext(frame_->width, frame_->height,
                                  static_cast<AVPixelFormat>(frame_->format),
                                  video_resolution_.width(), video_resolution_.height(),
                                  AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            SPDLOG_ERROR("Failed to get sws context.");
            return QImage();
        }
    }

    int linesizes[4];
    av_image_fill_linesizes(linesizes, AV_PIX_FMT_RGBA, frame_->width);

    uint8_t* image_buf[] = {image_buf_};
    sws_scale(sws_ctx_, frame_->data, frame_->linesize, 0, frame_->height, image_buf, linesizes);

    QImage image(image_buf_, frame_->width, frame_->height, QImage::Format_RGBA8888);
    av_frame_unref(frame_);

    return image;
}

void FFmpegDecoder::FFmpegError(int error_code)
{
    char error_buf[1024];
    av_strerror(error_code, error_buf, sizeof(error_buf));
    SPDLOG_ERROR("Error code: {} desc: {}", error_code, error_buf);
}

void FFmpegDecoder::InitHardWareDc() {}
