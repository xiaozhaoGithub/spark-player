#include "video_player_widget.h"

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent)
    , divide_num_(1)
{
    video_thread_ = new VideoWorkerThread(this);
    connect(video_thread_, &VideoWorkerThread::UpdateImage, this, &VideoPlayerWidget::UpdateImage);
    connect(video_thread_, &VideoWorkerThread::PlayState, this, &VideoPlayerWidget::PlayState);

    InitMenu();
}

VideoPlayerWidget::~VideoPlayerWidget() {}

void VideoPlayerWidget::Open(const char* name)
{
    video_thread_->Open();
}

void VideoPlayerWidget::Pause()
{
    video_thread_->Pause();
}

void VideoPlayerWidget::Stop()
{
    video_thread_->Stop();
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
