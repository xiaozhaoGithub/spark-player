#ifndef DEF_H_
#define DEF_H_

typedef enum
{
    kFile,
    kNetwork,
    kCapture,
    kNone
} MediaSourceType;

typedef enum
{
    PIX_FMT_YUV420P,
    PIX_FMT_YUVJ420P,
    PIX_FMT_YUVJ422P,
    PIX_FMT_NV12,
} PixelFormat;

#endif
