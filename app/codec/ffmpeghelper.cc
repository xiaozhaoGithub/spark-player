#include "ffmpeghelper.h"

#include "spdlog/spdlog.h"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

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

void FFmpegHelper::DecodeAudio(AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* pkt,
                               FILE* outfile_stream)
{
    int ret = avcodec_send_packet(codec_ctx, pkt);
    if (ret < 0) {
        FFmpegError(ret);
        return;
    }

    int data_size = 0;
    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            FFmpegError(ret);
            return;
        }

        // Note
        data_size = av_get_bytes_per_sample(codec_ctx->sample_fmt);
        if (data_size < 0) {
            FFmpegError(ret);
            return;
        }

        for (int i = 0; i < frame->nb_samples; ++i) {
            for (int ch = 0; ch < codec_ctx->channels; ++ch) {
                fwrite(frame->data[ch] + i * data_size, 1, data_size, outfile_stream);
            }
        }
    }
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
        return false;
    }

    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        FFmpegError(AVERROR(ENOMEM));
        return false;
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
        return false;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    av_dump_format(fmt_ctx, 0, filename, 0);

    return true;
}

static int get_format_from_sample_fmt(const char** fmt, enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry
    {
        enum AVSampleFormat sample_fmt;
        const char* fmt_be;
        const char* fmt_le;
    } sample_fmt_entries[] = {
        {AV_SAMPLE_FMT_U8, "u8", "u8"},        {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
        {AV_SAMPLE_FMT_S32, "s32be", "s32le"}, {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
        {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
    };
    *fmt = nullptr;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        sample_fmt_entry* entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    SPDLOG_ERROR("sample format %s is not supported as output format.",
                 av_get_sample_fmt_name(sample_fmt));

    return -1;
}

bool FFmpegHelper::SaveDecodeAudio(const char* filename, const char* outfile)
{
    if (!filename || *filename == '\0' || !outfile || *outfile == '\0')
        return false;

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!codec) {
        SPDLOG_ERROR("Failed to find Codec.");
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec) {
        SPDLOG_ERROR("Failed to alloc codec context.");
        return false;
    }

    AVCodecParserContext* parser_ctx = av_parser_init(codec->id);
    if (!parser_ctx) {
        SPDLOG_ERROR("Failed to init codec parser.");
        return false;
    }
    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    FILE* file_stream = fopen(filename, "rb");
    if (!file_stream) {
        SPDLOG_ERROR("Failed to open input file.");
        return false;
    }
    DEFER(fclose(file_stream);)

    FILE* outfile_stream = fopen(outfile, "wb");
    if (!outfile_stream) {
        av_free(codec_ctx);

        SPDLOG_ERROR("Failed to open output file.");
        return false;
    }
    DEFER(fclose(outfile_stream);)

    AVPacket* pkt = av_packet_alloc();

    AVFrame* decode_frame = nullptr;

    uint8_t inbuf[AUDIO_INBUF_SIZE + AUDIO_REFILL_THRESH];
    size_t data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, file_stream);

    uint8_t* data = inbuf;

    while (data_size > 0) {
        if (!decode_frame) {
            decode_frame = av_frame_alloc();
            if (!decode_frame) {
                DEFER(av_free_packet(pkt);)

                SPDLOG_ERROR("Failed to alloc audio frame.");
                return false;
            }
        }

        int outbuf_size = 0;
        int used_size = av_parser_parse2(parser_ctx, codec_ctx, &pkt->data, &pkt->size, data,
                                         data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        DecodeAudio(codec_ctx, decode_frame, pkt, outfile_stream);

        data += used_size;
        data_size -= used_size;

        if (data_size < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, data_size);
            data = inbuf;

            size_t len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, file_stream);
            if (len > 0) {
                data_size += len;
            }
        }
    }

    // flush the decoder.
    pkt->data = nullptr;
    pkt->size = 0;
    DecodeAudio(codec_ctx, nullptr, pkt, outfile_stream);

    AVSampleFormat sfmt = codec_ctx->sample_fmt;
    if (av_sample_fmt_is_planar(sfmt)) {
        const char* packed = av_get_sample_fmt_name(sfmt);
        SPDLOG_WARN("Warning: the sample format the decoder produced is planar "
                    "(%s). This example will output the first channel only.",
                    packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    DEFER(av_free_packet(pkt); avcodec_free_context(&codec_ctx); av_parser_close(parser_ctx);
          av_frame_free(&decode_frame); av_free_packet(pkt);)

    const char* fmt = nullptr;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0) {
        return false;
    }

    int channels = codec_ctx->channels;
    SPDLOG_INFO("Play the output audio file with the command:\n"
                "ffplay -f {0} -ac {1} -ar {2} {3}",
                fmt, channels, codec_ctx->sample_rate, outfile);

    return true;
}
