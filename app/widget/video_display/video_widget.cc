#include "video_widget.h"

#include "media_play/video_player_factory.h"
#include "render/render_factory.h"
#include "widget/common/fast_layout.h"

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
    InitUi();

    render_timer_ = new QTimer(this);
    render_timer_->setTimerType(Qt::PreciseTimer);
    connect(render_timer_, &QTimer::timeout, this, &VideoWidget::OnRender);
}

void VideoWidget::InitUi()
{
    render_wnd_ = RenderFactory::Create(kOpenGL, this);
    InitMenu();
}

void VideoWidget::InitMenu()
{
    menu_ = new QMenu(this);
    menu_->addAction(tr("Full Screen"), this, &VideoWidget::FullScreenClicked);
    menu_->addAction(tr("Exit Full Screen"), this, &VideoWidget::ExitFullScreenClicked);
}

void VideoWidget::Open()
{
    video_player_ = VideoPlayerFactory::Create(kFile);
    video_player_->Open();

    render_timer_->start();
}

void VideoWidget::Pause()
{
    video_player_->Pause();
}

void VideoWidget::Stop()
{
    video_player_->Stop();

    SAFE_FREE(video_player_);
}

void VideoWidget::StartRecord()
{
    // video_player_->StartRecord();
}

void VideoWidget::StopRecord()
{
    // video_player_->StopRecord();
}

void VideoWidget::set_media(const MediaInfo& media)
{
    video_player_->set_media(media);
}

void VideoWidget::resizeEvent(QResizeEvent* event)
{
    render_wnd_->setGeometry(rect());
}

void VideoWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);

    menu_->popup(mapToGlobal(event->pos()));
}

void VideoWidget::OnRender()
{
    DecodeFrame frame;
    bool pop_ok = video_player_->pop_frame(&frame);
    if (!pop_ok)
        return;

    render_wnd_->Render(frame);
}

void VideoWidget::FullScreenClicked()
{
    if (isFullScreen())
        return;

    setWindowFlags(Qt::Window);

    // Direct full screen will only be full screen on the main screen and placed in the Qt queue.
    QMetaObject::invokeMethod(this, "showFullScreen", Qt::QueuedConnection);
}

void VideoWidget::ExitFullScreenClicked()
{
    if (!isFullScreen())
        return;

    setWindowFlags(Qt::Widget);
    showNormal();
}
