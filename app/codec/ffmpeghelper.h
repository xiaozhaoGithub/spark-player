#ifndef FFMPEGHELPER_H_
#define FFMPEGHELPER_H_

#include <mutex>

extern "C"
{
//#include "libavcodec/avcodec.h"
//#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/file.h"
}

class FFmpegHelper
{
public:
    explicit FFmpegHelper();
    ~FFmpegHelper();

    static void FFmpegError(int code);

    static bool ReadMediaByAvio(const char* filename);

private:
    struct BufferData
    {
        uint8_t* ptr;
        size_t size;
    };

    static int read_packet(void* opaque, uint8_t* buf, int buf_size);
};

#endif
