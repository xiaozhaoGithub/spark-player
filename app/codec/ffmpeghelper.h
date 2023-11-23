#ifndef FFMPEGHELPER_H_
#define FFMPEGHELPER_H_

#include <functional>
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/file.h"
}

using Function = std::function<void()>;

// same as golang defer
class Defer
{
public:
    Defer(Function&& fn)
        : _fn(std::move(fn))
    {}
    ~Defer()
    {
        if (_fn)
            _fn();
    }

private:
    Function _fn;
};

#define STRINGCAT(x, y) STRINGCAT_HELPER(x, y)
#define STRINGCAT_HELPER(x, y) x##y

#define DEFER(code) Defer STRINGCAT(_defer_, __LINE__)([&]() { code });

class FFmpegHelper
{
public:
    explicit FFmpegHelper();
    ~FFmpegHelper();

    static void FFmpegError(int code);

    static bool ReadMediaByAvio(const char* filename);

    static bool SaveDecodeAudio(const char* infile, const char* outfile);
    static bool SaveEncodeAudio(const char* infile, const char* outfile);

private:
    struct BufferData
    {
        uint8_t* ptr;
        size_t size;
    };

    static int read_packet(void* opaque, uint8_t* buf, int buf_size);
    static void DecodeAudio(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile_fp);
    static int EncodeAudio(AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile_fp);
};

#endif
