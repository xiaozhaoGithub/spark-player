#include "ffmpegdecoder.h"

#include "common/singleton.h"
#include "config/config.h"
#include "spdlog/spdlog.h"
#include "util/cthread.h"
#include "widget/video_display/video_display_widget.h"

FFmpegDecoder::FFmpegDecoder()
    : fmt_ctx_(nullptr)
    , codec_ctx_(nullptr)
    , sws_ctx_(nullptr)
    , video_stream_(nullptr)
    , packet_(nullptr)
    , frame_(nullptr)
    , hw_frame_(nullptr)
    , hw_dev_ctx_(nullptr)
{
    InitDecodeParams();

    // Find hardware codec devices.
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
        hw_devices_.push_back(type);
    }

    // Register input device.
    avdevice_register_all();
}

FFmpegDecoder::~FFmpegDecoder()
{
    Close();
}

bool FFmpegDecoder::Open()
{
    std::string url;
    AVInputFormat* input_fmt = nullptr;
    if (!InitInputFmtParams(url, &input_fmt))
        return false;

    AVDictionary* dict = nullptr;
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);
    av_dict_set(&dict, "max_delay", "3", 0);
    av_dict_set(&dict, "timeout", "1000000", 0);

    int error_code = avformat_open_input(&fmt_ctx_, url.data(), input_fmt, &dict);
    if (error_code != 0) {
        SPDLOG_ERROR("Failed to open input stream.");
        return false;
    }

    if (dict) {
        av_dict_free(&dict);
    }

    av_dump_format(fmt_ctx_, 0, url.data(), 0);

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

    video_stream_ = fmt_ctx_->streams[video_index];

    fps_ = static_cast<int>(av_q2d(video_stream_->avg_frame_rate));
    frame_num_ = video_stream_->nb_frames;

    const AVCodec* codec = avcodec_find_decoder(video_stream_->codecpar->codec_id);
    if (!codec) {
        SPDLOG_ERROR("Failed to find Codec.");
        return false;
    }
    SPDLOG_INFO("resolution: [w:{0}, h:{1}] frame rate:{2} total frames:{3} codec name:{4}",
                video_stream_->codecpar->width, video_stream_->codecpar->height, fps_, frame_num_,
                codec->name);

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        SPDLOG_ERROR("Failed to create video decoder context.");
        return false;
    }

    error_code = avcodec_parameters_to_context(codec_ctx_, video_stream_->codecpar);
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
        Singleton<Config>::Instance()->AppConfigData("video_param", "enable_hw_decode", false).toBool();
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

    if (!AllocResource()) {
        return false;
    }

    int src_w = codec_ctx_->width;
    int src_h = codec_ctx_->height;

    int dst_w = src_w >> 2 << 2;
    int dst_h = src_h;

    std::string dst_pix_fmt_str =
        Singleton<Config>::Instance()->AppConfigData("video_param", "dst_pix_fmt").toString().toStdString();

    AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;
    if (!dst_pix_fmt_str.empty()) {
        if (dst_pix_fmt_str.compare("YUV") == 0) {
            dst_pix_fmt = AV_PIX_FMT_YUV420P;
        } else if (dst_pix_fmt_str.compare("RGB") == 0) {
            dst_pix_fmt = AV_PIX_FMT_RGB24;
        }
    }

    if (!sws_ctx_) {
        sws_ctx_ = sws_getContext(src_w, src_h, codec_ctx_->pix_fmt, dst_w, dst_h, dst_pix_fmt,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) {
            SPDLOG_ERROR("Failed to get sws context.");
            return false;
        }
    }

    // Default alloc
    decode_frame_.buf.resize(dst_w * dst_h * 4);

    if (dst_pix_fmt == AV_PIX_FMT_YUV420P) {
        int y_size = dst_w * dst_h;
        decode_frame_.buf.len = y_size * 3 / 2;
        data_[0] = reinterpret_cast<uint8_t*>(decode_frame_.buf.base);
        data_[1] = data_[0] + y_size;
        data_[2] = data_[1] + y_size / 4;
        linesize_[0] = dst_w;
        linesize_[1] = dst_w / 2;
        linesize_[2] = dst_w / 2;
    } else {
        decode_frame_.buf.len = dst_w * dst_h * 3;
        data_[0] = reinterpret_cast<uint8_t*>(decode_frame_.buf.base);
        linesize_[0] = dst_w * 3;
    }

    end_ = false;

    return true;
}

void FFmpegDecoder::Close()
{
    FreeResource();
}

int FFmpegDecoder::GetPacket(AVPacket* pkt)
{
    int ret;

    do {
        av_packet_unref(pkt);
        ret = av_read_frame(fmt_ctx_, pkt);
    } while (pkt->stream_index != video_stream_->index && ret >= 0);

    return ret;
}

int GetCommonFmt(int format)
{
    switch (format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        return PIX_FMT_IYUV;
    case AV_PIX_FMT_YUVJ422P:
        return PIX_FMT_YUVJ422P;
    case AV_PIX_FMT_NV12:
        return PIX_FMT_NV12;
    default:
        return 0;
    }
}

DecodeFrame* FFmpegDecoder::GetFrame()
{
    if (!fmt_ctx_)
        return nullptr;

    int ret = GetPacket(packet_);
    if (ret == 0) {
        if (packet_->stream_index == video_stream_->index) {
            packet_->pts = qRound64(packet_->pts * (1000 * av_q2d(video_stream_->time_base)));
            packet_->dts = qRound64(packet_->dts * (1000 * av_q2d(video_stream_->time_base)));

            if (avcodec_send_packet(codec_ctx_, packet_) != 0) {
                FFmpegError(ret);
            }
        }
    } else if (ret == AVERROR_EOF) {
        avcodec_send_packet(codec_ctx_, nullptr); // Flush decoder
    } else {
        FFmpegError(ret);
    }
    av_packet_unref(packet_);

    int error_code = avcodec_receive_frame(codec_ctx_, frame_);
    if (error_code == AVERROR(EAGAIN) || error_code == AVERROR_EOF) {
        av_frame_unref(frame_);

        if (ret < 0) {
            end_ = true;
        }
        return nullptr;
    }

    // The first byte of hw data is nullptr.
    AVFrame* tmp_frame = frame_;
    if (!frame_->data[0]) {
        tmp_frame = hw_frame_;
        bool success = GpuDataToCpu();
        av_frame_unref(frame_);

        if (!success) {
            return nullptr;
        }
    }

    int out_h = sws_scale(sws_ctx_, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height,
                          data_, linesize_);
    if (out_h <= 0 || out_h != tmp_frame->height) {
        return nullptr;
    }
    decode_frame_.w = tmp_frame->width;
    decode_frame_.h = tmp_frame->height;
    decode_frame_.format = GetCommonFmt(tmp_frame->format);
    decode_frame_.ts = tmp_frame->pts * av_q2d(video_stream_->time_base) * 1000;

    return &decode_frame_;
}

void FFmpegDecoder::FFmpegError(int error_code)
{
    char error_buf[1024];
    av_strerror(error_code, error_buf, sizeof(error_buf));
    SPDLOG_ERROR("Error code: {0} desc: {1}", error_code, error_buf);
}

bool FFmpegDecoder::AllocResource()
{
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

    return true;
}

void FFmpegDecoder::FreeResource()
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

    if (hw_dev_ctx_) {
        av_buffer_unref(&hw_dev_ctx_);
    }
}

bool FFmpegDecoder::InitInputFmtParams(std::string& url, AVInputFormat** fmt)
{
    switch (media_.type) {
    case kCapture: {
#if defined(Q_OS_WIN)
        // You can't turn on the webcam if you don't have it on Windows
        *fmt = av_find_input_format("dshow");
#elif defined(Q_OS_LINUX)
        *fmt = av_find_input_format("video4linux2");
#elif defined(Q_OS_MAC)
        *fmt = av_find_input_format("avfoundation");
#endif
        if (!*fmt) {
            SPDLOG_ERROR("Failed to get this input device.");
            return false;
        }

        url = "video=" + media_.src;
        break;
    }
    case kFile:
    case kNetwork: {
        url = media_.src;
        break;
    }
    default:
        SPDLOG_ERROR("Failed to open this media type {0}.", (int)media_.type);
        return false;
    }

    return true;
}

void FFmpegDecoder::InitDecodeParams()
{
    video_duration_ = 0;
    fps_ = 0;
    frame_num_ = 0;
    end_ = true;
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
            auto iter = std::find(hw_devices_.begin(), hw_devices_.end(), hw_config->device_type);
            if (iter != hw_devices_.end()) {
                int ret =
                    av_hwdevice_ctx_create(&hw_dev_ctx_, hw_config->device_type, nullptr, nullptr, 0);
                if (ret != 0) {
                    SPDLOG_ERROR("Failed to open specified type of hw device or create ctx.");
                    return;
                }

                SPDLOG_INFO("Open the hardware device type as {0}",
                            av_hwdevice_get_type_name(hw_config->device_type));

                codec_ctx_->hw_device_ctx = av_buffer_ref(hw_dev_ctx_);
                codec_ctx_->opaque = (void*)(&hw_config->pix_fmt);
                codec_ctx_->get_format = get_hw_format;
                return;
            }
        }
    }
}

bool FFmpegDecoder::GpuDataToCpu()
{
    const AVPixelFormat* format = static_cast<const AVPixelFormat*>(codec_ctx_->opaque);
    if (frame_->format != *format) {
        return false;
    }

    // int ret = av_hwframe_transfer_data(hw_frame_, frame_, 0);
    int ret = av_hwframe_map(hw_frame_, frame_, 0);
    if (ret != 0) {
        FFmpegError(ret);
        return false;
    }

    hw_frame_->width = frame_->width;
    hw_frame_->height = frame_->height;

    // Copy frame meta data.
    av_frame_copy_props(hw_frame_, frame_);

    return true;
}

AVPixelFormat FFmpegDecoder::get_hw_format(AVCodecContext* ctx, const AVPixelFormat* fmt)
{
    const AVPixelFormat* pix_fmt = static_cast<const AVPixelFormat*>(ctx->opaque);

    for (auto p = fmt; *p != -1; p++) {
        if (*p == *pix_fmt) {
            return *p;
        }
    }

    SPDLOG_ERROR("Failed to get hardware pixel format, AV_PIX_FMT_NONE will return.");
    return AV_PIX_FMT_NONE;
}
