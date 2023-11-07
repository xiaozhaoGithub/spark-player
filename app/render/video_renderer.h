#ifndef VIDEO_RENDERER_H_
#define VIDEO_RENDERER_H_

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QTime>

#include "codec/ffmpegdecoder.h"

class VideoRenderWorker;
class VideoRenderer : public QObject
{
    Q_OBJECT
public:
    explicit VideoRenderer(QObject* parent = nullptr);
    ~VideoRenderer();

    void Open(const char* name);
    void Pause();
    void Stop();

signals:
    void UpdateImage(const QImage& image);
    void PlayState(bool state);

private:
    QThread* thread_;
    std::unique_ptr<VideoRenderWorker> worker_;
};

class VideoRenderWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoRenderWorker(QObject* parent = nullptr);
    ~VideoRenderWorker();

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

    inline void set_decode_mode(bool is_soft);

signals:
    void UpdateImage(const QImage& image);
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

inline QByteArray VideoRenderWorker::filename()
{
    return filename_;
}

inline void VideoRenderWorker::set_filename(QByteArray filename)
{
    filename_ = filename;
}

inline VideoRenderWorker::VideoPlayState VideoRenderWorker::playstate()
{
    return playstate_;
}

inline void VideoRenderWorker::set_playstate(VideoPlayState state)
{
    playstate_ = state;
}

inline void VideoRenderWorker::set_decode_mode(bool is_soft)
{
    decoder_->set_decode_mode(is_soft);
}

#endif
