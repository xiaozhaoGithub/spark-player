#include "video_renderer.h"

VideoRenderer::VideoRenderer(QObject* parent)
    : QObject(parent)
{
    thread_ = new QThread(this);
    thread_->setObjectName("VideoRenderWorkerThread");

    worker_ = std::make_unique<VideoRenderWorker>();
    worker_->moveToThread(thread_);
    connect(worker_.get(), &VideoRenderWorker::UpdateImage, this, &VideoRenderer::UpdateImage,
            Qt::DirectConnection);
    connect(worker_.get(), &VideoRenderWorker::PlayState, this, &VideoRenderer::PlayState);

    thread_->start();
}

VideoRenderer::~VideoRenderer()
{
    Stop();

    thread_->quit();
    thread_->wait();
}

void VideoRenderer::Open(const char* name)
{
    if (worker_->filename() == name && worker_->playstate() == VideoRenderWorker::kPause) {
        worker_->set_playstate(VideoRenderWorker::kPlaying);
        return;
    }

    worker_->set_filename(name);
    QMetaObject::invokeMethod(worker_.get(), "Run", Qt::QueuedConnection);
}

void VideoRenderer::Pause()
{
    worker_->set_playstate(VideoRenderWorker::kPause);
}

void VideoRenderer::Stop()
{
    worker_->set_playstate(VideoRenderWorker::kStop);
}


VideoRenderWorker::VideoRenderWorker(QObject* parent)
    : QObject(parent)
{
    decoder_ = new FFmpegDecoder(this);
}

VideoRenderWorker::~VideoRenderWorker() {}

void VideoRenderWorker::Run()
{
    bool ret = decoder_->Open(filename_);
    if (!ret)
        return;

    emit PlayState(true);

    playstate_ = kPlaying;
    elapsed_timer_.start();

    while (playstate_ != kStop) {
        while (playstate_ == kPause) {
            QThread::msleep(200);
        }

        QImage frame_image = decoder_->GetFrame();
        if (!frame_image.isNull()) {
            int time = elapsed_timer_.elapsed();
            int pts = decoder_->pts();
            int ms = pts - time;
            if (ms > 0) {
                QThread::msleep(ms);
            }

            emit UpdateImage(frame_image);
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
