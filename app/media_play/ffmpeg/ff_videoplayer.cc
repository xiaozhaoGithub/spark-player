#include "ff_videoplayer.h"

#include <QApplication>

#include "common/avdef.h"
#include "media_play/stream_event_type.h"
#include "spdlog/spdlog.h"

FFVideoPlayer::FFVideoPlayer(QObject* parent)
    : VideoPlayer()
    , CThread(parent)
    , decoder_(new FFmpegProcessor)
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

void FFVideoPlayer::StartRecord(const char* file)
{
    if (writer_)
        return;

    writer_ = std::make_unique<FFmpegWriter>();

    auto media_info = media();
    media_info.src = file;
    writer_->set_media(media_info);
}

void FFVideoPlayer::StopRecord()
{
    if (!writer_)
        return;

    writer_->Stop();
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
    decoder_->Start();
}

void FFVideoPlayer::DoFinish()
{
    decoder_->Close();

    event_cb(kStreamClose);
}

void FFVideoPlayer::DoRecordTask(DecodeFrame* frame)
{
    if (!writer_) {
        return;
    }

    if (frame) {
        if (!writer_->opened()) {
            bool opened = writer_->Open(*decoder_->encode_data_info());
            if (!opened) {
                writer_->Stop();
            }
        }

        if (writer_->opened()) {
            writer_->Write(*frame);
        }
    }

    if (writer_->is_stop()) {
        if (writer_->opened()) {
            writer_->Close();
        }

        writer_.reset();
    }
}
