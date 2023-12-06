#include "ff_videoplayer.h"

#include <QApplication>

#include "common/avdef.h"
#include "spdlog/spdlog.h"

FFVideoPlayer::FFVideoPlayer(QObject* parent)
    : VideoPlayer()
    , QThread(parent)
    , playstate_(kStop)
{
    decoder_ = std::make_unique<FFmpegDecoder>();
    writer_ = std::make_unique<FFmpegWriter>();
}

FFVideoPlayer::~FFVideoPlayer()
{
    set_playstate(FFVideoPlayer::kStop);

    quit();
    wait();
}

void FFVideoPlayer::Open()
{
    if (playstate() == FFVideoPlayer::kPause) {
        set_playstate(FFVideoPlayer::kPlaying);
        return;
    }

    start();
}

void FFVideoPlayer::Pause()
{
    set_playstate(FFVideoPlayer::kPause);
}

void FFVideoPlayer::Stop()
{
    set_playstate(FFVideoPlayer::kStop);
}

void FFVideoPlayer::Resume() {}

void FFVideoPlayer::StartRecord()
{
    QString filename =
        QApplication::applicationDirPath() + "/"
        + QString("video_record_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    MediaInfo media;
    media.type = decoder_->media()->type;
    media.src = filename.toStdString();
    // media.type = kCapture;
    // media.src = "desktop";

    writer_->set_media(media);

    if (writer_->Open(decoder_->video_stream())) {
        emit RecordState(true);
    }
}

void FFVideoPlayer::StopRecord()
{
    writer_->Close();
    emit RecordState(false);
}

void FFVideoPlayer::run()
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

        DecodeFrame* frame = decoder_->GetFrame();
        if (frame) {
            if (decoder_->media()->type != kCapture) {
                int ms = decoder_->pts() - elapsed_timer_.elapsed();
                if (ms > 0) {
                    QThread::msleep(ms);
                }
            }
            // writer_->Write(frame);
            push_frame(frame);
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
