#include "ff_videoplayer.h"

#include <QApplication>

#include "common/avdef.h"
#include "media_play/stream_event_type.h"
#include "spdlog/spdlog.h"

FFVideoPlayer::FFVideoPlayer(QObject* parent)
    : VideoPlayer()
    , CThread(parent)
    , playstate_(kStop)
{
    decoder_ = std::make_unique<FFmpegDecoder>();
    writer_ = std::make_unique<FFmpegWriter>();
}

FFVideoPlayer::~FFVideoPlayer()
{
    Stop();
}

void FFVideoPlayer::Start()
{
    if (playstate_ == FFVideoPlayer::kPause) {
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

    quit();
    wait();
}

void FFVideoPlayer::Resume()
{
    if (playstate_ == FFVideoPlayer::kPause) {
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
    if (!ret) {
        event_cb(kOpenStreamFail);
        return;
    }

    fps_ = decoder_->fps();
    set_sleep_policy(kUntil, 1000 / decoder_->fps());

    event_cb(kOpenStreamSuccess);

    playstate_ = kPlaying;
    while (playstate_ != kStop) {
        while (playstate_ == kPause) {
            std::this_thread::yield();
        }

        DecodeFrame* frame = decoder_->GetFrame();
        if (frame) {
            // writer_->Write(frame);
            push_frame(frame);

            CThread::Sleep();
        } else {
            if (decoder_->is_end()) {
                playstate_ = kStop;
                event_cb(kStreamEnd);
            }
        }
    }

    decoder_->Close();
    writer_->Close();

    event_cb(kStreamClose);
}
