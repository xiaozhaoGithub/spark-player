#ifndef DECODE_FRAME_H_
#define DECODE_FRAME_H_

#include "common/base_interface.h"
#include "common/bufs.h"

class DecodeFrame
{
public:
    DecodeFrame();
    ~DecodeFrame();

    HBuf buf_;
    int w;
    int h;
};

DecodeFrame::DecodeFrame() {}

DecodeFrame::~DecodeFrame() {}
#endif
