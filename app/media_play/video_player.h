#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include "common/media_info.h"
#include "util/decode_frame_buf.h"

class VideoPlayer
{
public:
    VideoPlayer();
    virtual ~VideoPlayer();

    enum VideoPlayState
    {
        kPlaying,
        kPause,
        kStop
    };

    virtual void Open() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual void Resume() = 0;

    MediaInfo media() { return media_; }
    void set_media(const MediaInfo& media) { media_ = media; }

    VideoPlayState playstate() { return playstate_; }
    void set_playstate(VideoPlayState state) { playstate_ = state; }

    bool push_frame(DecodeFrame* frame) { return frame_buf_.Push(frame); };
    bool pop_frame(DecodeFrame* frame) { return frame_buf_.Pop(frame); }

private:
    VideoPlayState playstate_;
    DecodeFrameBuf frame_buf_;

    MediaInfo media_;
};

#endif
