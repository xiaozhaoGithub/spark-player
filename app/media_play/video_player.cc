#include "video_player.h"

VideoPlayer::VideoPlayer()
    : playstate_(kStop)
    , fps_(0)
{}

VideoPlayer::~VideoPlayer() {}

void VideoPlayer::event_cb(StreamEventType ev)
{
    if (event_cb_)
        event_cb_(ev);
}
