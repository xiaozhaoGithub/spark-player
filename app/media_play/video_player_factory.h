#ifndef VIDEO_PLAYER_FACTORY_H_
#define VIDEO_PLAYER_FACTORY_H_

#include "video_player.h"
#include "common/avdef.h"
#include "ffmpeg/ff_videoplayer.h"

class VideoPlayerFactory
{
public:
    static VideoPlayer* Create(MediaSourceType type)
    {
        switch (type) {
        case kFile:
        case kNetwork:
        case kCapture:
            return new FFVideoPlayer;
        case kNone:
        default:
            return nullptr;
        }
    }
};

#endif
