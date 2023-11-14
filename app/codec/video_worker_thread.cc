#include "video_worker_thread.h"

VideoWorkerThread::VideoWorkerThread(QObject* parent)
    : QThread(parent)
{
    decoder_ = new FFmpegDecoder(this);
    writer_ = std::make_unique<FFmpegWriter>();
}

VideoWorkerThread::~VideoWorkerThread()
{
    set_playstate(VideoWorkerThread::kStop);

    quit();
    wait();
}

void VideoWorkerThread::Open()
{
    if (playstate() == VideoWorkerThread::kPause) {
        set_playstate(VideoWorkerThread::kPlaying);
        return;
    }

    start();
}

void VideoWorkerThread::Pause()
{
    set_playstate(VideoWorkerThread::kPause);
}

void VideoWorkerThread::Stop()
{
    set_playstate(VideoWorkerThread::kStop);
}

void VideoWorkerThread::StartRecord()
{
    QString filename =
        QString("video_record_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    bool ret = writer_->Open(filename.toUtf8().data(), decoder_->stream());

    if (ret) {
        emit RecordState(true);
    }
}

void VideoWorkerThread::StopRecord()
{
    writer_->Close();
    emit RecordState(false);
}

void VideoWorkerThread::run()
{
    bool ret = decoder_->Open();
    if (!ret)
        return;

    emit PlayState(true);

    playstate_ = kPlaying;
    elapsed_timer_.start();

    while (playstate_ != kStop) {
        while (playstate_ == kPause) {
            QThread::msleep(200);
        }

        AVFrame* frame = decoder_->GetFrame();
        if (frame) {
            if (decoder_->media()->type != kCapture) {
                int ms = decoder_->pts() - elapsed_timer_.elapsed();
                if (ms > 0) {
                    QThread::msleep(ms);
                }
            }
            writer_->Write(frame);

            emit SendFrame(frame);
        } else {
            if (decoder_->is_end()) {
                playstate_ = kStop;
            }
            QThread::msleep(1);
        }
    }

    decoder_->Close();
    writer_->Close();

    emit PlayState(false);
}
