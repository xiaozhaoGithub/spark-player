#ifndef DECODE_FRAME_BUF_H_
#define DECODE_FRAME_BUF_H_

#include <deque>
#include <mutex>

#include "decode_frame.h"

struct FrameState
{
    uint32_t push_cnt = 0;
    uint32_t pop_cnt = 0;
    uint32_t push_ok_cnt = 0;
    uint32_t pop_ok_cnt = 0;
};

class DecodeFrameBuf : public HRingBuf
{
public:
    DecodeFrameBuf();
    ~DecodeFrameBuf();

    void set_cache(int cache_num) { cache_num_ = cache_num; }

    bool Push(DecodeFrame* frame);
    bool Pop(DecodeFrame* frame);

private:
    std::deque<DecodeFrame> frames_;
    int cache_num_;

    std::mutex mutex_;

    FrameState frame_state_;
};

#endif
