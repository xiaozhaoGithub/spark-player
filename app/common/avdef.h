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
    PIX_FMT_IYUV,
    PIX_FMT_YUVJ422P,
    PIX_FMT_NV12,
    PIX_FMT_RGB,
} PixelFormat;

typedef enum
{
    H264,
    HEVC,
    MJPEG,
} EncodeFormat;


typedef struct
{
    EncodeFormat compression;
    int w;
    int h;
    int fps;
} EncodeDataInfo;

#endif
