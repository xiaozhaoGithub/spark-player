#ifndef MEDIA_INFO_
#define MEDIA_INFO_

#include <string>

#include "common/avdef.h"

typedef struct media_info_
{
    MediaSourceType type;
    std::string src;

    media_info_() { type = kNone; };
} MediaInfo;
#endif
