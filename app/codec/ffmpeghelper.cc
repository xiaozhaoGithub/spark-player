#include "ffmpeghelper.h"
extern "C"
{
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
}
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

void FFmpegHelper::EncodeVideo(AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile_fp)
{
    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        FFmpegError(ret);
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            FFmpegError(ret);
            return;
        }

        fwrite(pkt->data, 1, pkt->size, outfile_fp);

        av_packet_unref(pkt);
    }

    return;
}

void FFmpegHelper::DecodeVideo(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile_fp)
{
    int ret = avcodec_send_packet(codec_ctx, pkt);
    if (ret < 0) {
        FFmpegError(ret);
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            FFmpegError(ret);
            return;
        }

        // YUV420P
        fwrite(frame->data[0], 1, frame->width * frame->height, outfile_fp);
        fwrite(frame->data[1], 1, frame->width * frame->height / 4, outfile_fp);
        fwrite(frame->data[2], 1, frame->width * frame->height / 4, outfile_fp);

        av_frame_unref(frame);
    }

    return;
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

bool FFmpegHelper::SaveTranscodeFormat(const AvInfo& info, const char* infile, const char* outfile)
{
    AVFormatContext* infmt_ctx = nullptr;
    int ret = avformat_open_input(&infmt_ctx, infile, nullptr, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avformat_close_input(&infmt_ctx);)

    ret = avformat_find_stream_info(infmt_ctx, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    AVFormatContext* outfmt_ctx = nullptr;
    ret = avformat_alloc_output_context2(&outfmt_ctx, nullptr, nullptr, outfile);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avformat_free_context(outfmt_ctx);)

    // Copy all streams.(If you want, you can only operate part of the stream.)
    for (int i = 0; i < infmt_ctx->nb_streams; ++i) {
        AVStream* in_stream = infmt_ctx->streams[i];
        AVStream* out_stream = avformat_new_stream(outfmt_ctx, nullptr);
        if (!out_stream) {
            SPDLOG_ERROR("Failed to create out stream index[{0}]", i);
            continue;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if (ret < 0) {
            FFmpegError(ret);
            continue;
        }
        // Solve incompatible for codec
        out_stream->codecpar->codec_tag = 0;
    }

    // Open avio before writer header
    if (!(outfmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&outfmt_ctx->pb, outfile, AVIO_FLAG_WRITE, nullptr, nullptr);
        if (ret < 0) {
            FFmpegError(ret);
            return false;
        }
    }
    DEFER(avio_closep(&outfmt_ctx->pb);)

    ret = avformat_write_header(outfmt_ctx, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    DEFER(av_packet_free(&pkt);)

    // Seek frame
    ret = av_seek_frame(infmt_ctx, -1, info.start_time * AV_TIME_BASE, AVSEEK_FLAG_ANY);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    int64_t* start_pts = new int64_t[infmt_ctx->nb_streams];
    memset(start_pts, 0, infmt_ctx->nb_streams * sizeof(int64_t));

    int64_t* start_dts = new int64_t[infmt_ctx->nb_streams];
    memset(start_dts, 0, infmt_ctx->nb_streams * sizeof(int64_t));

    DEFER(delete[] start_pts; delete[] start_dts;)

    while (av_read_frame(infmt_ctx, pkt) == 0) {
        AVStream* in_stream = infmt_ctx->streams[pkt->stream_index];
        AVStream* out_stream = outfmt_ctx->streams[pkt->stream_index];

        if (info.end_time * in_stream->time_base.den < pkt->pts) {
            av_packet_unref(pkt);
            break;
        };

        // Init start pts and dts, due to cropping.
        if (start_pts[pkt->stream_index] == 0) {
            start_pts[pkt->stream_index] = pkt->pts;
        }
        if (start_dts[pkt->stream_index] == 0) {
            start_dts[pkt->stream_index] = pkt->dts;
        }

        // Different encapsulation stream timebase may be different and timestamp need to be converted.
        pkt->pts -= start_pts[pkt->stream_index];
        pkt->dts -= start_dts[pkt->stream_index];
        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base); // pts, dts, duration...
        if (pkt->pts < 0) {
            pkt->pts = 0;
        }
        if (pkt->dts < 0) {
            pkt->dts = 0;
        }
        pkt->pos = -1;

        if (pkt->dts > pkt->pts) {
            av_packet_unref(pkt);
            continue;
        }

        ret = av_interleaved_write_frame(outfmt_ctx, pkt);
        if (ret < 0) {
            FFmpegError(ret);
            av_packet_unref(pkt);
            break;
        }

        av_packet_unref(pkt);
    }

    ret = av_write_trailer(outfmt_ctx);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

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
    DEFER(av_packet_free(&pkt););

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
    AVFormatContext* fmt_ctx = nullptr;
    int ret = avformat_open_input(&fmt_ctx, infile, nullptr, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avformat_close_input(&fmt_ctx););

    // Fill stream info
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(nullptr);
    if (!codec_ctx) {
        SPDLOG_ERROR("Failed to alloc codec context.");
        return false;
    }
    DEFER(avcodec_free_context(&codec_ctx););

    int video_index = ret;
    AVStream* stream = fmt_ctx->streams[video_index];

    AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        SPDLOG_ERROR("Failed to find codec.");
        return false;
    }

    ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avcodec_close(codec_ctx););

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    DEFER(av_packet_free(&pkt););

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }
    DEFER(av_frame_free(&frame););

    FILE* outfile_fp = fopen(outfile, "wb");
    if (!outfile_fp) {
        SPDLOG_ERROR("Failed to open outfile.");
        return false;
    }
    DEFER(fclose(outfile_fp););


    // Write frame to outfile
    while (ret >= 0) {
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            FFmpegError(ret);
            return false;
        }

        if (pkt->stream_index != video_index) {
            continue;
        }

        DecodeVideo(codec_ctx, pkt, frame, outfile_fp);

        av_packet_unref(pkt);
    }
    // Flush decoder
    DecodeVideo(codec_ctx, nullptr, frame, outfile_fp);

    return true;
}

bool FFmpegHelper::SaveEncodeVideo(const AvInfo& info, const char* infile, const char* outfile)
{
    AVCodec* codec = avcodec_find_encoder_by_name(info.codec_name.data());
    if (!codec) {
        SPDLOG_ERROR("Failed to find video codec {0}.", info.codec_name);
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        SPDLOG_ERROR("Failed to alloc codec context.");
        return false;
    }
    DEFER(avcodec_free_context(&codec_ctx);)

    int width;
    int height;
    int ret = av_parse_video_size(&width, &height, info.video_size.data());
    if (ret < 0) {
        SPDLOG_ERROR("Failed to parse video size:{0}.", info.video_size);
        return false;
    }

    // Encoder params
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->framerate = {info.framerate, 1};
    codec_ctx->time_base = {1, info.framerate};
    codec_ctx->bit_rate = info.bit_rate;
    codec_ctx->gop_size = info.gop_size;
    codec_ctx->max_b_frames = info.max_b_frames;

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
    }

    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avcodec_close(codec_ctx);)

    // Intput and output file
    FILE* infile_fp = fopen(infile, "rb");
    if (!infile_fp) {
        SPDLOG_ERROR("Failed to open infile.");
        return false;
    }
    DEFER(fclose(infile_fp););

    FILE* outfile_fp = fopen(outfile, "wb");
    if (!outfile_fp) {
        SPDLOG_ERROR("Failed to open output file.");
        return false;
    }
    DEFER(fclose(outfile_fp);)


    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    DEFER(av_packet_free(&pkt);)

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        SPDLOG_ERROR("Failed to alloc frame.");
        return false;
    }
    DEFER(av_frame_free(&frame);)

    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;

    // Malloc image buffer
    // yuv420p: w * h * 3/2
    uint8_t* frame_buffer = (uint8_t*)av_malloc(
        av_image_get_buffer_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1));
    DEFER(av_freep(&frame_buffer);)

    // Fill data and linesize
    av_image_fill_arrays(frame->data, frame->linesize, frame_buffer, codec_ctx->pix_fmt,
                         codec_ctx->width, codec_ctx->height, 1);

    SPDLOG_INFO("Start video encoding encoder:{0}, video size:[{1}x{2}], pixel format:{3}.",
                codec->name, width, height, (int)codec_ctx->pix_fmt);

    int video_size = width * height;

    int frame_num = 0;
    while (fread(frame_buffer, 1, video_size * 3 / 2, infile_fp) == video_size * 3 / 2) {
        frame->data[0] = frame_buffer;
        frame->data[1] = frame_buffer + video_size;
        frame->data[2] = frame_buffer + video_size + video_size / 4;
        // Presentation time order in encoder.
        frame->pts = frame_num++;

        EncodeVideo(codec_ctx, frame, pkt, outfile_fp);
    }
    EncodeVideo(codec_ctx, nullptr, pkt, outfile_fp);

    // Add sequence end code to have a real MPEG file
    uint8_t endcode[] = {0, 0, 1, 0xb7};
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO) {
        fwrite(endcode, 1, sizeof(endcode), outfile_fp);
    }

    return true;
}

bool FFmpegHelper::ExportSingleStream(int media_type, const char* infile, const char* outfile)
{
    AVFormatContext* infmt_ctx = nullptr;
    int ret = avformat_open_input(&infmt_ctx, infile, nullptr, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avformat_close_input(&infmt_ctx););

    // Fill stream info
    ret = avformat_find_stream_info(infmt_ctx, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    ret = av_find_best_stream(infmt_ctx, static_cast<AVMediaType>(media_type), -1, -1, nullptr, 0);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    int dump_index = ret;

    AVFormatContext* outfmt_ctx = nullptr;
    ret = avformat_alloc_output_context2(&outfmt_ctx, nullptr, nullptr, outfile);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    DEFER(avformat_free_context(outfmt_ctx);)

    if (!(outfmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outfmt_ctx->pb, outfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            FFmpegError(ret);
            return false;
        }
    }
    DEFER(avio_closep(&outfmt_ctx->pb);)

    // Copy stream params
    AVStream* in_stream = infmt_ctx->streams[dump_index];
    AVStream* out_stream = avformat_new_stream(outfmt_ctx, nullptr);
    if (!out_stream) {
        SPDLOG_ERROR("Failed to create out stream");
        return false;
    }

    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }
    // Solve incompatible for codec
    out_stream->codecpar->codec_tag = 0;

    ret = avformat_write_header(outfmt_ctx, nullptr);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        SPDLOG_ERROR("Failed to alloc packet.");
        return false;
    }
    DEFER(av_packet_free(&pkt);)

    while (av_read_frame(infmt_ctx, pkt) == 0) {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            FFmpegError(ret);
            return false;
        }

        if (pkt->stream_index != dump_index) {
            av_packet_unref(pkt);
            continue;
        }

        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base); // pts, dts, duration...
        pkt->pos = -1;
        pkt->stream_index = 0;

        ret = av_interleaved_write_frame(outfmt_ctx, pkt);
        if (ret < 0) {
            FFmpegError(ret);
            av_packet_unref(pkt);
            break;
        }

        av_packet_unref(pkt);
    }

    ret = av_write_trailer(outfmt_ctx);
    if (ret < 0) {
        FFmpegError(ret);
        return false;
    }

    return true;
}
