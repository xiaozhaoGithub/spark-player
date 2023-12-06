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

    decoder_->set_media(media());

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

void FFVideoPlayer::Resume()
{
    if (playstate() == FFVideoPlayer::kPause) {
        set_playstate(FFVideoPlayer::kPlaying);
    }
}

void FFVideoPlayer::StartRecord()
{
    QString filename =
        QApplication::applicationDirPath() + "/"
        + QString("video_record_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    MediaInfo media;
    media.type = decoder_->media().type;
    media.src = filename.toStdString();

    writer_->set_media(media);
    writer_->Open(decoder_->video_stream());
}

void FFVideoPlayer::StopRecord()
{
    writer_->Close();
    // emit RecordState(false);
}

void FFVideoPlayer::run()
{
    bool ret = decoder_->Open();
    if (!ret)
        return;

    playstate_ = kPlaying;
    while (playstate_ != kStop) {
        while (playstate_ == kPause) {
            QThread::msleep(200);
        }

        DecodeFrame* frame = decoder_->GetFrame();
        if (frame) {
            // writer_->Write(frame);
            int per_frame_ms = 1000 / frame->fps;
            push_frame(frame);

            /*    std::chrono::time_point();
                std::this_thread::sleep_until();*/
        } else {
            if (decoder_->is_end()) {
                playstate_ = kStop;
            }
            QThread::msleep(1);
        }
    }

    decoder_->Close();
    writer_->Close();
}
