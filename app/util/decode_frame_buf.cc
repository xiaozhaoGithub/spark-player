#include "decode_frame_buf.h"

#define DEFAUT_FRAME_CACHE_NUM 10

DecodeFrameBuf::DecodeFrameBuf()
    : cache_num_(DEFAUT_FRAME_CACHE_NUM)
{}

DecodeFrameBuf::~DecodeFrameBuf() {}

bool DecodeFrameBuf::Push(DecodeFrame* frame)
{
    ++frame_state_.push_cnt;

    if (frame->IsNull())
        return false;

    std::lock_guard<std::mutex> lock_guard(mutex_);

    // dropped frame
    if (frames_.size() >= cache_num_) {
        DecodeFrame& frame = frames_.front();
        frames_.pop_front();
        free(frame.buf.len);
    }

    if (isNull()) {
        resize(cache_num_ * frame->buf.len);
    }

    DecodeFrame cache_frame;
    cache_frame.buf.base = alloc(frame->buf.len);
    cache_frame.buf.len = frame->buf.len;
    cache_frame.Copy(*frame);

    frames_.emplace_back(cache_frame);

    ++frame_state_.push_ok_cnt;

    return true;
}

bool DecodeFrameBuf::Pop(DecodeFrame* frame)
{
    --frame_state_.pop_cnt;

    std::lock_guard<std::mutex> lock_guard(mutex_);

    if (frames_.size() == 0) {
        return false;
    }

    auto cache_frame = frames_.front();
    frames_.pop_front();

    free(cache_frame.buf.len);

    frame->Copy(cache_frame);

    --frame_state_.pop_ok_cnt;

    return true;
}
