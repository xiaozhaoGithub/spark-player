#ifndef MEDIA_INFO_
#define MEDIA_INFO_

#include <string>

#include "common/def.h"

typedef struct MediaInfo_
{
    MediaSourceType type;
    std::string src;

    MediaInfo_()
    {
        type = kNone;
    };
} MediaInfo;
#endif
