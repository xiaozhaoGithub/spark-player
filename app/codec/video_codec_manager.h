#ifndef VIDEO_CODEC_MANAGER_
#define VIDEO_CODEC_MANAGER_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QTime>

#include "codec/ffmpegdecoder.h"

class VideoCodecWorker;
class VideoCodecManager : public QObject
{
    Q_OBJECT
public:
    explicit VideoCodecManager(QObject* parent = nullptr);
    ~VideoCodecManager();

    void Open(const char* name);
    void Pause();
    void Stop();

signals:
    void UpdateImage(const QImage& image);
    void SendFrame(AVFrame* frame);
    void PlayState(bool state);

private:
    QThread* thread_;
    std::unique_ptr<VideoCodecWorker> worker_;
};

class VideoCodecWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoCodecWorker(QObject* parent = nullptr);
    ~VideoCodecWorker();

    enum VideoPlayState
    {
        kPlaying,
        kPause,
        kStop
    };

    inline QByteArray filename();
    inline void set_filename(QByteArray filename);

    inline VideoPlayState playstate();
    inline void set_playstate(VideoPlayState state);

signals:
    void SendFrame(AVFrame* frame);
    void PlayState(bool state);

public Q_SLOTS:
    void Run();

private:
    FFmpegDecoder* decoder_;
    QTime elapsed_timer_;

    QByteArray filename_;
    VideoPlayState playstate_;
    bool soft_decode_;
};

inline QByteArray VideoCodecWorker::filename()
{
    return filename_;
}

inline void VideoCodecWorker::set_filename(QByteArray filename)
{
    filename_ = filename;
}

inline VideoCodecWorker::VideoPlayState VideoCodecWorker::playstate()
{
    return playstate_;
}

inline void VideoCodecWorker::set_playstate(VideoPlayState state)
{
    playstate_ = state;
}

#endif
