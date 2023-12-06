#include "decode_frame_buf.h"

DecodeFrameBuf::DecodeFrameBuf() {}

DecodeFrameBuf::~DecodeFrameBuf() {}

void DecodeFrameBuf::Push(DecodeFrame* frame)
{
    ++frame_state_.push_cnt;

    if (frame->IsNull())
        return;

    std::lock_guard<std::mutex> lock_guard(mutex_);

    // dropped frame
    if (frames_.size() > cache_num_) {
        frames_.pop_front();
    }

    if (isNull()) {
        resize(cache_num_ * frame->buf_.len);
    }

    DecodeFrame cache_frame;
    cache_frame.buf_.base = alloc(frame->buf_.len);
    cache_frame.Copy(*frame);

    frames_.emplace_back(cache_frame);

    ++frame_state_.push_ok_cnt;
}

void DecodeFrameBuf::Pop(DecodeFrame* frame)
{
    --frame_state_.pop_cnt;

    std::lock_guard<std::mutex> lock_guard(mutex_);

    if (frames_.size() == 0) {
        return;
    }

    auto cache_frame = frames_.front();
    frames_.pop_front();

    free(frame->buf_.len);

    frame->Copy(cache_frame);

    --frame_state_.pop_ok_cnt;
}
