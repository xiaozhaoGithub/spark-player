#include "ff_videoplayer.h"

#include <QApplication>

#include "common/avdef.h"
#include "media_play/stream_event_type.h"
#include "spdlog/spdlog.h"

FFVideoPlayer::FFVideoPlayer(QObject* parent)
    : VideoPlayer()
    , CThread(parent)
    , decoder_(new FFmpegDecoder)
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
    info_ = decoder_->encode_data_info();

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
            push_frame(frame);

            CThread::Sleep();
        } else {
            if (decoder_->end()) {
                set_state(kStop);
                StopRecord();

                event_cb(kStreamEnd);
            }
        }

        DoRecordTask(frame);
    }
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
            bool opened = writer_->Open(info_);
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
