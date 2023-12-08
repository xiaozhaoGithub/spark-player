#ifndef STREAM_EVENT_TYPE_H_
#define STREAM_EVENT_TYPE_H_

typedef enum
{
    kOpenStreamSuccess,
    kOpenStreamFail,
    kStreamEnd,
    kStreamClose,
    kStreamError
} StreamEventType;
#endif
