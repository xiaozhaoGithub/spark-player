#ifndef VIDEO_WORKER_THREAD_H_
#define VIDEO_WORKER_THREAD_H_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QTime>

#include "codec/ffmpegdecoder.h"
#include "codec/ffmpegwriter.h"
#include "common/media_info.h"

class VideoWorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoWorkerThread(QObject* parent = nullptr);
    ~VideoWorkerThread();

    enum VideoPlayState
    {
        kPlaying,
        kPause,
        kStop
    };

    inline void set_media(const MediaInfo& media);

    inline VideoPlayState playstate();
    inline void set_playstate(VideoPlayState state);

    void Open();
    void Pause();
    void Stop();

    void StartRecord();
    void StopRecord();

signals:
    void UpdateImage(const QImage& image);
    void SendFrame(AVFrame* frame);
    void PlayState(bool state);
    void RecordState(bool state);

protected:
    void run() override;

private:
    FFmpegDecoder* decoder_;
    std::unique_ptr<FFmpegWriter> writer_;

    QTime elapsed_timer_;

    QByteArray filename_;
    VideoPlayState playstate_;
};

inline VideoWorkerThread::VideoPlayState VideoWorkerThread::playstate()
{
    return playstate_;
}

inline void VideoWorkerThread::set_playstate(VideoPlayState state)
{
    playstate_ = state;
}

inline void VideoWorkerThread::set_media(const MediaInfo& media)
{
    decoder_->set_media(media);
}

#endif
