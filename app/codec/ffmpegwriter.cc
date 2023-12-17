#include "ffmpegwriter.h"

#include "ffmpeghelper.h"
#include "spdlog/spdlog.h"

FFmpegWriter::FFmpegWriter()
    : opened_(false)
    , stop_(false)
    , fmt_ctx_(nullptr)
    , codec_(nullptr)
    , codec_ctx_(nullptr)
    , video_stream_(nullptr)
    , packet_(nullptr)
    , header_written_(false)
    , frame_index_(0)
{}

FFmpegWriter::~FFmpegWriter() {}

void FFmpegWriter::set_media(const MediaInfo& media)
{
    media_ = media;
}

bool FFmpegWriter::Open(const EncodeDataInfo& info)
{
    const char* filename = media_.src.c_str();
    SPDLOG_INFO("Record filename: {0}", filename);

    AVCodecID codec_id = AV_CODEC_ID_NONE;
    AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    if (info.compression == H264) {
        codec_id = AV_CODEC_ID_H264;
    } else if (info.compression == HEVC) {
        codec_id = AV_CODEC_ID_HEVC;
    } else if (info.compression == MJPEG) {
        codec_id = AV_CODEC_ID_MJPEG;
        pix_fmt = AV_PIX_FMT_YUVJ420P;
    }

    codec_ = avcodec_find_encoder(codec_id);
    if (!codec_) {
        SPDLOG_ERROR("Failed to find encoder for id: {0}", codec_id);
        return false;
    }
    SPDLOG_INFO("Find the encoder name: {0}", codec_->name);

    int error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to alloc output context from file extension.");

        error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "mp4", filename);
        if (error_code) {
            SPDLOG_ERROR("Failed to alloc output context for mp4.");
            return false;
        }
    }

    error_code = avio_open(&fmt_ctx_->pb, filename, AVIO_FLAG_WRITE);
    if (error_code < 0) {
        SPDLOG_ERROR("Failed to open avio filename: {0}.", filename);
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_) {
        SPDLOG_ERROR("Failed to alloc codec context");
        return false;
    }

    // Set some necessary parameters before opening the encoder.
    codec_ctx_->bit_rate = 8500000;
    // Resolution must be a multiple of two
    codec_ctx_->width = info.w;
    codec_ctx_->height = info.h;
    codec_ctx_->time_base = {1, info.fps};
    codec_ctx_->framerate = {info.fps, 1};
    codec_ctx_->gop_size = 10;
    codec_ctx_->max_b_frames = 0;
    codec_ctx_->pix_fmt = pix_fmt;
    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    error_code = avcodec_open2(codec_ctx_, nullptr, nullptr);
    if (error_code < 0) {
        FFmpegHelper::FFmpegError(error_code);
        return false;
    }

    video_stream_ = avformat_new_stream(fmt_ctx_, nullptr);
    if (!video_stream_) {
        SPDLOG_ERROR("Failed to create new video stream.");
        return false;
    }

    error_code = avcodec_parameters_from_context(video_stream_->codecpar, codec_ctx_);
    if (error_code < 0) {
        FFmpegHelper::FFmpegError(error_code);
        return false;
    }

    // note
    error_code = avformat_write_header(fmt_ctx_, nullptr);
    if (error_code < 0) {
        FFmpegHelper::FFmpegError(error_code);
        return false;
    }
    header_written_ = true;

    frame_ = av_frame_alloc();
    if (!frame_) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }
    frame_->format = codec_ctx_->pix_fmt;
    frame_->width = codec_ctx_->width;
    frame_->height = codec_ctx_->height;
    av_frame_get_buffer(frame_, 1);

    packet_ = av_packet_alloc();
    if (!packet_) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }

    opened_ = true;

    return true;
}

bool FFmpegWriter::Write(const DecodeFrame& frame)
{
    frame_->pts = frame_index_++;

    av_image_fill_arrays(frame_->data, frame_->linesize, (const uint8_t*)frame.buf.base,
                         static_cast<AVPixelFormat>(frame_->format), frame_->width, frame_->height, 1);

    std::unique_lock<std::mutex> lock(mutex_);

    int ret = 0;
    if (frame.IsNull()) {
        ret = avcodec_send_frame(codec_ctx_, nullptr);
    } else {
        ret = avcodec_send_frame(codec_ctx_, frame_);
    }

    if (ret < 0) {
        FFmpegHelper::FFmpegError(ret);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            FFmpegHelper::FFmpegError(ret);
            return false;
        }

        av_packet_rescale_ts(packet_, codec_ctx_->time_base, video_stream_->time_base);
        ret = av_interleaved_write_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            FFmpegHelper::FFmpegError(ret);
            av_packet_unref(packet_);
            break;
        }

        av_packet_unref(packet_);
    }

    return true;
}

void FFmpegWriter::Close()
{
    // Flush encoder
    Write(DecodeFrame());

    std::unique_lock<std::mutex> lock(mutex_);

    if (fmt_ctx_) {
        if (header_written_) {
            header_written_ = false;

            int error_code = av_write_trailer(fmt_ctx_);
            if (error_code < 0) {
                FFmpegHelper::FFmpegError(error_code);
                return;
            }
            SPDLOG_INFO("Stop record.");
        }
        avio_close(fmt_ctx_->pb);
    }

    frame_index_ = 0;
    opened_ = false;

    FreeResource();
}

void FFmpegWriter::FreeResource()
{
    if (fmt_ctx_) {
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = nullptr;
    }

    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }

    if (frame_) {
        av_frame_free(&frame_);
    }

    if (packet_) {
        av_packet_free(&packet_);
    }
}
