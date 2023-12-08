#ifndef DECODE_FRAME_h
#define DECODE_FRAME_h

#include "common/base_interface.h"
#include "common/bufs.h"

class DecodeFrame
{
public:
    void Copy(const DecodeFrame& frame)
    {
        buf.copy(frame.buf.base, frame.buf.len);
        w = frame.w;
        h = frame.h;
        ts = frame.ts;
        format = frame.format;
    }

    bool IsNull() { return w == 0 || h == 0 || buf.isNull(); }

public:
    HBuf buf;
    int w;
    int h;
    uint64_t ts; // ms
    int format;
};

#endif
