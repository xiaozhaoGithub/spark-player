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
    // Find Hardware Codec Devices.
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

    bool enable_hw_dc =
        Singleton<Config>::Instance()->AppConfigData("video_param", "enable_hardware_dc", false).toBool();
    if (enable_hw_dc) {
        InitHwDecode(codec);
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

    hw_frame_ = av_frame_alloc();
    if (!hw_frame_) {
        SPDLOG_ERROR("Failed to alloc hw frame.");
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

    if (hw_frame_) {
        av_frame_free(&hw_frame_);
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

    // The first byte of hw data is nullptr.
    AVFrame* tmp_frame = frame_;
    if (!frame_->data[0]) {
        tmp_frame = hw_frame_;
        bool ret = GpuDataToCpu();
        av_frame_unref(frame_);

        if (!ret) {
            return QImage();
        }
    }

    pts_ = tmp_frame->pts;

    if (!sws_ctx_) {
        sws_ctx_ = sws_getContext(tmp_frame->width, tmp_frame->height,
                                  static_cast<AVPixelFormat>(tmp_frame->format),
                                  video_resolution_.width(), video_resolution_.height(),
                                  AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            SPDLOG_ERROR("Failed to get sws context.");
            return QImage();
        }
    }

    int linesizes[4];
    av_image_fill_linesizes(linesizes, AV_PIX_FMT_RGBA, tmp_frame->width);

    uint8_t* image_buf[] = {image_buf_};
    sws_scale(sws_ctx_, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height, image_buf,
              linesizes);

    QImage image(image_buf_, tmp_frame->width, tmp_frame->height, QImage::Format_RGBA8888);
    av_frame_unref(tmp_frame);

    return image;
}

void FFmpegDecoder::FFmpegError(int error_code)
{
    char error_buf[1024];
    av_strerror(error_code, error_buf, sizeof(error_buf));
    SPDLOG_ERROR("Error code: {0} desc: {1}", error_code, error_buf);
}

void FFmpegDecoder::InitHwDecode(const AVCodec* codec)
{
    for (int i = 0;; i++) {
        const AVCodecHWConfig* hw_config = avcodec_get_hw_config(codec, i);
        if (!hw_config) {
            SPDLOG_ERROR("Failed to get codec hardware configuration");
            return;
        }

        if (hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
            if (hw_devices.contains(hw_config->device_type)) {
                int ret =
                    av_hwdevice_ctx_create(&hw_dev_ctx_, hw_config->device_type, nullptr, nullptr, 0);
                if (ret != 0) {
                    SPDLOG_ERROR("Failed to open specified type of hw device or create ctx.");
                    return;
                }
                codec_ctx_->hw_device_ctx = av_buffer_ref(hw_dev_ctx_);
                codec_ctx_->opaque = (void*)(&hw_config->pix_fmt);
                codec_ctx_->get_format = get_hw_format;
            }
        }
    }
}

bool FFmpegDecoder::GpuDataToCpu()
{
    // av_hwframe_transfer_data((hw_frame_, frame_, 0);
    int ret = av_hwframe_map(hw_frame_, frame_, 0);
    if (ret != 0) {
        FFmpegError(ret);
        return false;
    }

    // copy frame meta data.
    av_frame_copy_props(hw_frame_, frame_);

    return true;
}

AVPixelFormat FFmpegDecoder::get_hw_format(AVCodecContext* ctx, const AVPixelFormat* fmt)
{
    AVPixelFormat* pix_fmt = static_cast<AVPixelFormat*>(ctx->opaque);

    for (auto p = fmt; *p != -1; p++) {
        if (*p == *pix_fmt) {
            return *p;
        }
    }

    SPDLOG_ERROR("Failed to get hardware pixel format, AV_PIX_FMT_NONE will return.");
    return AV_PIX_FMT_NONE;
}
