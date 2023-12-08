#ifndef FF_VIDEO_PLAYER_H_
#define FF_VIDEO_PLAYER_H_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QTime>

#include "codec/ffmpegdecoder.h"
#include "codec/ffmpegwriter.h"
#include "common/media_info.h"
#include "media_play/video_player.h"
#include "util/cthread.h"

class FFVideoPlayer : public VideoPlayer, public CThread
{
public:
    explicit FFVideoPlayer(QObject* parent = nullptr);
    ~FFVideoPlayer();

    VideoPlayState playstate() { return playstate_; }
    void set_playstate(VideoPlayState state) { playstate_ = state; }

    void Open() override;
    void Pause() override;
    void Stop() override;
    void Resume() override;

    void StartRecord();
    void StopRecord();

protected:
    void run() override;

private:
    std::unique_ptr<FFmpegDecoder> decoder_;
    std::unique_ptr<FFmpegWriter> writer_;

    QByteArray filename_;
    VideoPlayState playstate_;
};

#endif
