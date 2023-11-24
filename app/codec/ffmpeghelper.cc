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

void FFmpegHelper::DecodeAudio(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile_fp)
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
                fwrite(frame->data[ch] + i * data_size, 1, data_size, outfile_fp);
            }
        }
    }
}

int FFmpegHelper::EncodeAudio(AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile_fp)
{
    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        FFmpegError(ret);
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return ret;
        } else if (ret < 0) {
            FFmpegError(ret);
            return ret;
        }

        ret = fwrite(pkt->data, 1, pkt->size, outfile_fp);

        av_packet_unref(pkt);
    }

    return ret;
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

bool FFmpegHelper::SaveDecodeAudio(const char* infile, const char* outfile)
{
    if (!infile || *infile == '\0' || !outfile || *outfile == '\0')
        return false;

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    if (!codec) {
        SPDLOG_ERROR("Failed to find Codec.");
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        SPDLOG_ERROR("Failed to alloc codec context.");
        return false;
    }
    DEFER(avcodec_free_context(&codec_ctx);)

    AVCodecParserContext* parser_ctx = av_parser_init(codec->id);
    if (!parser_ctx) {
        SPDLOG_ERROR("Failed to init codec parser.");
        return false;
    }
    DEFER(av_parser_close(parser_ctx);)

    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    FILE* infile_fp = fopen(infile, "rb");
    if (!infile_fp) {
        SPDLOG_ERROR("Failed to open input file.");
        return false;
    }
    DEFER(fclose(infile_fp);)

    FILE* outfile_fp = fopen(outfile, "wb");
    if (!outfile_fp) {
        av_free(codec_ctx);

        SPDLOG_ERROR("Failed to open output file.");
        return false;
    }
    DEFER(fclose(outfile_fp);)

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    DEFER(av_packet_unref(pkt);)

    AVFrame* decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }
    DEFER(av_frame_free(&decoded_frame);)

    uint8_t inbuf[AUDIO_INBUF_SIZE + AUDIO_REFILL_THRESH];
    uint8_t* data_buf = inbuf;

    size_t data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, infile_fp);
    while (data_size > 0) {
        int outbuf_size = 0;
        int used_size = av_parser_parse2(parser_ctx, codec_ctx, &pkt->data, &pkt->size, data_buf,
                                         data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (used_size < 0) {
            return false;
            SPDLOG_ERROR("Error while parsing.");
        }

        data_buf += used_size;
        data_size -= used_size;

        if (pkt->size > 0) {
            DecodeAudio(codec_ctx, pkt, decoded_frame, outfile_fp);
        }

        if (data_size < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data_buf, data_size);
            data_buf = inbuf;

            size_t len = fread(data_buf + data_size, 1, AUDIO_INBUF_SIZE - data_size, infile_fp);
            if (len > 0) {
                data_size += len;
            }
        }
    }

    // Flush the decoder, otherwise the last few frames may be lost.
    pkt->data = nullptr;
    pkt->size = 0;
    DecodeAudio(codec_ctx, pkt, decoded_frame, outfile_fp);

    AVSampleFormat sfmt = codec_ctx->sample_fmt;
    if (av_sample_fmt_is_planar(sfmt)) {
        const char* packed = av_get_sample_fmt_name(sfmt);
        SPDLOG_WARN("Warning: the sample format the decoder produced is planar {0}. This example "
                    "will output the first channel only.",
                    packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

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

bool FFmpegHelper::SaveEncodeAudio(const char* infile, const char* outfile)
{
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!codec) {
        SPDLOG_ERROR("Failed to find audio codec.");
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        SPDLOG_ERROR("Failed to alloc codec context.");
        return false;
    }
    DEFER(avcodec_free_context(&codec_ctx););

    codec_ctx->bit_rate = 320000;
    codec_ctx->sample_rate = 44100;
    codec_ctx->channels = 2;
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16P;

    int ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avcodec_close(codec_ctx););

    FILE* infile_fp = fopen(infile, "rb");
    if (!infile_fp) {
        SPDLOG_ERROR("Failed to open infile.");
        return false;
    }
    DEFER(fclose(infile_fp););


    FILE* outfile_fp = fopen(outfile, "wb");
    if (!outfile_fp) {
        SPDLOG_ERROR("Failed to open outfile.");
        return false;
    }
    DEFER(fclose(outfile_fp););


    // Frame to packet
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }
    DEFER(av_frame_free(&frame););

    frame->sample_rate = codec_ctx->sample_rate;
    frame->format = codec_ctx->sample_fmt;
    frame->channels = codec_ctx->channels;
    frame->channel_layout = codec_ctx->channel_layout;
    frame->nb_samples = codec_ctx->frame_size; // encoding: set by libavcodec in avcodec_open2()

    av_frame_get_buffer(frame, 0);

    while (true) {
        size_t read_byte = fread(frame->data[0], 1, frame->linesize[0], infile_fp);
        if (read_byte <= 0) {
            if (feof(infile_fp)) {
                SPDLOG_INFO("Data reading complete.");
            } else if (ferror(infile_fp)) {
                SPDLOG_ERROR("Failed to read packet.");
            }
            break;
        }
        EncodeAudio(codec_ctx, frame, pkt, outfile_fp);
    }
    EncodeAudio(codec_ctx, nullptr, pkt, outfile_fp);

    return true;
}

bool FFmpegHelper::SaveDecodeVideo(const char* infile, const char* outfile)
{
    return false;
}

bool FFmpegHelper::SaveEncodeVideo(const char* infile, const char* outfile)
{
    return false;
}
