#include "video_codec_manager.h"

VideoCodecManager::VideoCodecManager(QObject* parent)
    : QObject(parent)
{
    thread_ = new QThread(this);
    thread_->setObjectName("VideoCodecWorkerThread");

    worker_ = std::make_unique<VideoCodecWorker>();
    worker_->moveToThread(thread_);
    connect(worker_.get(), &VideoCodecWorker::SendFrame, this, &VideoCodecManager::SendFrame,
            Qt::BlockingQueuedConnection);
    connect(worker_.get(), &VideoCodecWorker::PlayState, this, &VideoCodecManager::PlayState);

    thread_->start();
}

VideoCodecManager::~VideoCodecManager()
{
    Stop();

    thread_->quit();
    thread_->wait();
}

void VideoCodecManager::Open()
{
    if (worker_->playstate() == VideoCodecWorker::kPause) {
        worker_->set_playstate(VideoCodecWorker::kPlaying);
        return;
    }

    QMetaObject::invokeMethod(worker_.get(), "Run", Qt::QueuedConnection);
}

void VideoCodecManager::Pause()
{
    worker_->set_playstate(VideoCodecWorker::kPause);
}

void VideoCodecManager::Stop()
{
    worker_->set_playstate(VideoCodecWorker::kStop);
}

void VideoCodecManager::set_media(const MediaInfo& media)
{
    worker_->set_media(media);
}


VideoCodecWorker::VideoCodecWorker(QObject* parent)
    : QObject(parent)
{
    decoder_ = new FFmpegDecoder(this);
}

VideoCodecWorker::~VideoCodecWorker() {}

void VideoCodecWorker::Run()
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

            emit SendFrame(frame);
        } else {
            if (decoder_->is_end()) {
                playstate_ = kStop;
            }
            QThread::msleep(1);
        }
    }

    decoder_->Close();

    emit PlayState(false);
}
