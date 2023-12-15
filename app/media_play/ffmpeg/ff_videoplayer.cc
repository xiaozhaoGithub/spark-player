#include "ff_videoplayer.h"

#include <QApplication>

#include "common/avdef.h"
#include "media_play/stream_event_type.h"
#include "spdlog/spdlog.h"

FFVideoPlayer::FFVideoPlayer(QObject* parent)
    : VideoPlayer()
    , CThread(parent)
    , decoder_(new FFmpegDecoder)
    , writer_(new FFmpegWriter)
{}

FFVideoPlayer::~FFVideoPlayer()
{
    Stop();
}

void FFVideoPlayer::Start()
{
    decoder_->set_media(media());

    start();
}

void FFVideoPlayer::Pause()
{
    set_state(kPause);
}

void FFVideoPlayer::Stop()
{
    set_state(kStop);

    quit();
    wait(); // Secure exit
}

void FFVideoPlayer::Resume()
{
    if (state() == kPause) {
        set_state(kRunning);
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

bool FFVideoPlayer::DoPrepare()
{
    bool ret = decoder_->Open();
    if (!ret) {
        event_cb(kOpenStreamFail);
        return false;
    }

    fps_ = decoder_->fps();
    set_sleep_policy(kUntil, 1000 / fps_);
    set_state(kRunning);

    event_cb(kOpenStreamSuccess);

    return true;
}

void FFVideoPlayer::DoTask()
{
    while (state() != kStop) {
        while (state() == kPause) {
            std::this_thread::yield();
        }

        DecodeFrame* frame = decoder_->GetFrame();
        if (frame) {
            // writer_->Write(frame);
            push_frame(frame);

            CThread::Sleep();
        } else {
            if (decoder_->is_end()) {
                set_state(kStop);

                event_cb(kStreamEnd);
            }
        }
    }
}

void FFVideoPlayer::DoFinish()
{
    decoder_->Close();
    writer_->Close();

    event_cb(kStreamClose);
}
