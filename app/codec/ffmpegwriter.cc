#include "ffmpegwriter.h"

#include "spdlog/spdlog.h"

FFmpegWriter::FFmpegWriter()
    : fmt_ctx_(nullptr)
    , codec_(nullptr)
    , codec_ctx_(nullptr)
    , new_stream_(nullptr)
    , packet_(nullptr)
    , header_written_(false)
    , frame_index_(0)
{}

FFmpegWriter::~FFmpegWriter()
{
    Close();
}

void FFmpegWriter::set_media(const MediaInfo& media)
{
    media_.reset(new MediaInfo(media));
}

bool FFmpegWriter::Open(AVStream* stream)
{
    if (!media_)
        return false;

    const char* filename = media_->src.data();
    SPDLOG_INFO("Record filename: {0}", filename);

    codec_ = avcodec_find_encoder(stream->codecpar->codec_id);
    if (!codec_) {
        SPDLOG_ERROR("Failed to find encoder for id: {0}", stream->codecpar->codec_id);
        return false;
    }
    SPDLOG_INFO("Find the encoder name: {0}", codec_->name);

    int error_code = 0;

    switch (media_->type) {
    case kCapture:
        error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, codec_->name, filename);
        break;
    case kFile:
        error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "h264", filename);
        break;
    case kNetwork:
        error_code = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename);
        break;
    default:
        SPDLOG_ERROR("Failed to open this media type {0}.", media_->type);
        return false;
    }

    if (error_code < 0) {
        FFmpegError(error_code);
        return false;
    }

    error_code = avio_open(&fmt_ctx_->pb, filename, AVIO_FLAG_WRITE);
    if (error_code < 0) {
        FFmpegError(error_code);
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_) {
        SPDLOG_ERROR("Failed to alloc codec context");
        return false;
    }

    // Set some necessary parameters before opening the encoder.
    codec_ctx_->width = stream->codecpar->width;
    codec_ctx_->height = stream->codecpar->height;
    codec_ctx_->time_base = {1, 10};
    codec_ctx_->framerate = {10, 1};
    codec_ctx_->bit_rate = 4000000;
    codec_ctx_->pix_fmt = (AVPixelFormat)stream->codecpar->format;
    codec_ctx_->gop_size = 10;
    // codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    error_code = avcodec_open2(codec_ctx_, nullptr, nullptr);
    if (error_code < 0) {
        FFmpegError(error_code);
        return false;
    }

    new_stream_ = avformat_new_stream(fmt_ctx_, nullptr);
    if (!new_stream_) {
        SPDLOG_ERROR("Failed to create new stream.");
        return false;
    }

    error_code = avcodec_parameters_from_context(new_stream_->codecpar, codec_ctx_);
    if (error_code < 0) {
        FFmpegError(error_code);
        return false;
    }

    error_code = avformat_write_header(fmt_ctx_, nullptr);
    if (error_code < 0) {
        FFmpegError(error_code);
        return false;
    }

    header_written_ = true;

    return AllocResource();
}

bool FFmpegWriter::Write(AVFrame* frame)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!packet_)
        return false;

    if (frame) {
        frame->pts = frame_index_++;
    }

    avcodec_send_frame(codec_ctx_, frame);

    while (true) {
        int error_code = avcodec_receive_packet(codec_ctx_, packet_);
        if (error_code < 0) {
            FFmpegError(error_code);
            break;
        }

        av_packet_rescale_ts(packet_, codec_ctx_->time_base, new_stream_->time_base);
        av_write_frame(fmt_ctx_, packet_);
        av_packet_unref(packet_);
    }

    return true;
}

void FFmpegWriter::Close()
{
    // Write null frame to represent end flag.
    Write(nullptr);

    std::unique_lock<std::mutex> lock(mutex_);

    if (fmt_ctx_) {
        if (header_written_) {
            header_written_ = false;

            int error_code = av_write_trailer(fmt_ctx_);
            if (error_code < 0) {
                FFmpegError(error_code);
                return;
            }
            SPDLOG_INFO("Stop record.");
        }
        avio_close(fmt_ctx_->pb);
    }

    frame_index_ = 0;
    FreeResource();
}

void FFmpegWriter::FFmpegError(int error_code)
{
    char error_buf[1024];
    av_strerror(error_code, error_buf, sizeof(error_buf));
    SPDLOG_ERROR("Error code: {0} desc: {1}", error_code, error_buf);
}

bool FFmpegWriter::AllocResource()
{
    packet_ = av_packet_alloc();
    if (!packet_) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }

    return true;
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

    if (packet_) {
        av_packet_free(&packet_);
    }
}
