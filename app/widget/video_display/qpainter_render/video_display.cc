#include "video_display.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent)
    , divide_num_(1)
{
    video_renderer_ = new VideoRenderer(this);
    connect(video_renderer_, &VideoRenderer::UpdateImage, this, &VideoPlayerWidget::UpdateImage,
            Qt::DirectConnection);
    connect(video_renderer_, &VideoRenderer::PlayState, this, &VideoPlayerWidget::PlayState);

    InitMenu();
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

void VideoPlayerWidget::FullScreenClicked()
{
    if (isFullScreen())
        return;

    setWindowFlags(Qt::Window);

    // Direct full screen will only be full screen on the main screen and placed in the Qt queue.
    QMetaObject::invokeMethod(this, "showFullScreen", Qt::QueuedConnection);
}

void VideoPlayerWidget::ExitFullScreenClicked()
{
    if (!isFullScreen())
        return;

    setWindowFlags(Qt::Widget);
    showNormal();
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

void VideoPlayerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void VideoPlayerWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);

    menu_->popup(mapToGlobal(event->pos()));
}

void VideoPlayerWidget::InitMenu()
{
    menu_ = new QMenu(this);
    menu_->addAction(tr("full screen"), this, &VideoPlayerWidget::FullScreenClicked);
    menu_->addAction(tr("exit full screen"), this, &VideoPlayerWidget::ExitFullScreenClicked);
}
