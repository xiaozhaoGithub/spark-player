#ifndef DECODE_FRAME_BUF_H_
#define DECODE_FRAME_BUF_H_

#include <deque>

#include "decode_frame.h"

class DecodeFrameBuf
{
public:
    DecodeFrameBuf();
    ~DecodeFrameBuf();

private:
    std::deque<DecodeFrame> frames_;
};

#endif
