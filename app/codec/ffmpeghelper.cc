#include "ffmpeghelper.h"

#include "spdlog/spdlog.h"

FFmpegHelper::FFmpegHelper() {}

FFmpegHelper::~FFmpegHelper() {}

void FFmpegHelper::FFmpegError(int code)
{
    char error_buf[1024];
    av_strerror(code, error_buf, sizeof(error_buf));
    SPDLOG_ERROR("Error code: {0} desc: {1}", code, error_buf);
}

int FFmpegHelper::read_packet(void* opaque, uint8_t* buf, int buffer_size)
{
    BufferData* buf_data = static_cast<BufferData*>(opaque);
    int read_size = FFMIN(buffer_size, buf_data->size);

    if (read_size <= 0) {
        SPDLOG_INFO("read done, ptr: {0}, size: {1}", (void*)buf_data->ptr, buf_data->size);
        return AVERROR_EOF;
    }

    SPDLOG_INFO("ptr: {0}, size: {1}", (void*)buf_data->ptr, buf_data->size);

    memcpy(buf, buf_data->ptr, read_size);
    buf_data->ptr += read_size;
    buf_data->size -= read_size;

    if (buf_data->size == 0) {
        SPDLOG_INFO("read done, ptr: {0}, size: {1}", (void*)buf_data->ptr, buf_data->size);
    }

    return read_size;
}

bool FFmpegHelper::ReadMediaByAvio(const char* filename)
{
    if (!filename || *filename == '\0')
        return false;

    uint8_t* buf = nullptr;
    size_t buf_size;

    AVFormatContext* fmt_ctx = nullptr;
    AVIOContext* avio_ctx = nullptr;

    std::shared_ptr<char> defer(new char, [&](char* c) {
        delete c;

        avformat_close_input(&fmt_ctx);

        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);
        }
        avio_context_free(&avio_ctx);

        av_file_unmap(buf, buf_size);
    });

    int ret = av_file_map(filename, &buf, &buf_size, 0, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
    }

    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        FFmpegError(AVERROR(ENOMEM));
    }

    int avio_ctx_buf_size = 4096;
    uint8_t* avio_ctx_buf = (uint8_t*)av_malloc(avio_ctx_buf_size);

    BufferData buf_data;
    buf_data.ptr = buf;
    buf_data.size = buf_size;

    avio_ctx = avio_alloc_context(avio_ctx_buf, avio_ctx_buf_size, 0, &buf_data, read_packet,
                                  nullptr, nullptr);
    if (!avio_ctx) {
        FFmpegError(AVERROR(ENOMEM));
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        FFmpegError(ret);
    }

    av_dump_format(fmt_ctx, 0, filename, 0);
}
