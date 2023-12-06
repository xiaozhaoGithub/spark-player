#ifndef FF_VIDEO_PLAYER_H_
#define FF_VIDEO_PLAYER_H_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QTime>

#include "codec/ffmpegdecoder.h"
#include "codec/ffmpegwriter.h"
#include "common/media_info.h"
#include "media_play/video_player.h"

class FFVideoPlayer : public VideoPlayer, public QThread
{
    Q_OBJECT
public:
    explicit FFVideoPlayer(QObject* parent = nullptr);
    ~FFVideoPlayer();

    inline VideoPlayState playstate();
    inline void set_playstate(VideoPlayState state);

    void Open() override;
    void Pause() override;
    void Stop() override;
    void Resume() override;

    void StartRecord();
    void StopRecord();

signals:
    void UpdateImage(const QImage& image);
    void PlayState(bool state);
    void RecordState(bool state);

protected:
    void run() override;

private:
    std::unique_ptr<FFmpegDecoder> decoder_;
    std::unique_ptr<FFmpegWriter> writer_;

    QTime elapsed_timer_;

    QByteArray filename_;
    VideoPlayState playstate_;
};

inline FFVideoPlayer::VideoPlayState FFVideoPlayer::playstate()
{
    return playstate_;
}

inline void FFVideoPlayer::set_playstate(VideoPlayState state)
{
    playstate_ = state;
}

#endif