#ifndef DECODE_FRAME_H_
#define DECODE_FRAME_H_

#include "common/base_interface.h"
#include "common/bufs.h"

class DecodeFrame
{
public:
    DecodeFrame();
    ~DecodeFrame();

    void Copy(const DecodeFrame& frame)
    {
        buf_.copy(frame.buf_.base, frame.buf_.len);
        w = frame.w;
        h = frame.h;
    }

    bool IsNull() { return w == 0 || h == 0 || buf_.isNull(); }

public:
    HBuf buf_;
    int w;
    int h;
};

DecodeFrame::DecodeFrame() {}

DecodeFrame::~DecodeFrame() {}
#endif
