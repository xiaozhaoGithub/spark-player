#include "video_display.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent)
{
    video_renderer_ = new VideoRenderer(this);
    connect(video_renderer_, &VideoRenderer::UpdateImage, this, &VideoPlayerWidget::UpdateImage,
            Qt::DirectConnection);
    connect(video_renderer_, &VideoRenderer::PlayState, this, &VideoPlayerWidget::PlayState);
}

VideoPlayerWidget::~VideoPlayerWidget() {}

void VideoPlayerWidget::Open(const char* name)
{
    video_renderer_->Open(name);
}

void VideoPlayerWidget::Pause()
{
    video_renderer_->Pause();
}

void VideoPlayerWidget::Stop()
{
    video_renderer_->Stop();
}

void VideoPlayerWidget::set_pixmap(const QPixmap& pixmap)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pixmap_ = pixmap;
    }

    update();
}

void VideoPlayerWidget::UpdateImage(const QImage& image)
{
    set_pixmap(QPixmap::fromImage(image));
}

void VideoPlayerWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QPixmap pixmap;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!pixmap_.isNull()) {
            pixmap = pixmap_.scaled(this->size(), Qt::KeepAspectRatio);
        }
    }

    if (!pixmap.isNull()) {
        painter.drawPixmap(this->rect(), pixmap);
    }
}
