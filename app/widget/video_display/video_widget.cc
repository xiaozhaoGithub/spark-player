#include "video_widget.h"

#include <functional>

#include "common/singleton.h"
#include "config/config.h"
#include "media_play/video_player_factory.h"
#include "render/render_factory.h"
#include "widget/common/fast_layout.h"

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    , video_player_(nullptr)
    , play_state_(kStop)
    , fps_(0)
{
    InitUi();

    fps_ = Singleton<Config>::Instance()->AppConfigData("video_param", "fps").toInt();

    render_timer_ = new QTimer(this);
    render_timer_->setTimerType(Qt::PreciseTimer);
    connect(render_timer_, &QTimer::timeout, this, &VideoWidget::OnRender);
}

void VideoWidget::Open(const MediaInfo& media)
{
    media_ = media;
    Start();
}

void VideoWidget::Pause()
{
    if (!video_player_ || play_state_ != kRunning)
        return;

    play_state_ = kPause;
    render_timer_->stop();

    video_player_->Pause();
}

void VideoWidget::Stop()
{
    if (!video_player_ || play_state_ == kStop)
        return;

    play_state_ = kStop;
    video_player_->Stop();
}

void VideoWidget::StartRecord()
{
    // video_player_->StartRecord();
}

void VideoWidget::StopRecord()
{
    // video_player_->StopRecord();
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

void VideoWidget::InitUi()
{
    qRegisterMetaType<StreamEventType>("StreamEventType");

    render_wnd_ = RenderFactory::Create(kOpenGL, this);
    InitMenu();
}

void VideoWidget::InitMenu()
{
    menu_ = new QMenu(this);
    menu_->addAction(tr("Full Screen"), this, &VideoWidget::FullScreenClicked);
    menu_->addAction(tr("Exit Full Screen"), this, &VideoWidget::ExitFullScreenClicked);
}

void VideoWidget::Start()
{
    if (!video_player_) {
        video_player_ = VideoPlayerFactory::Create(media_.type);

        video_player_->set_media(media_);
        video_player_->set_event_cb(
            std::bind(&VideoWidget::StreamEventCallback, this, std::placeholders::_1));

        video_player_->Start();
    } else {
        Resume();
    }
}

void VideoWidget::Resume()
{
    if (play_state_ != kPause)
        return;

    if (!video_player_)
        return;

    int duration = 1000 / (fps_ ? fps_ : video_player_->fps());
    render_timer_->start(duration);
    video_player_->Resume();

    play_state_ = kRunning;
}

void VideoWidget::StreamEventCallback(StreamEventType type)
{
    QMetaObject::invokeMethod(this, "OnEventProcess", Qt::QueuedConnection,
                              Q_ARG(StreamEventType, type));
}

void VideoWidget::OnEventProcess(StreamEventType type)
{
    switch (type) {
    case kOpenStreamSuccess:
        OnOpenStreamSuccess();
        break;
    case kOpenStreamFail:
        OnOpenStreamFail();
        break;
    case kStreamEnd:
        OnStreamEnd();
        break;
    case kStreamClose:
        OnStreamClose();
        break;
    case kStreamError:
        OnStreamError();
        break;
    default:
        break;
    }
}

void VideoWidget::OnOpenStreamSuccess()
{
    play_state_ = kRunning;

    int duration = 1000 / (fps_ ? fps_ : video_player_->fps());
    render_timer_->start(duration);
}

void VideoWidget::OnOpenStreamFail()
{
    QMessageBox::warning(this, tr("Warning"), tr("Failed to open stream"));
}

void VideoWidget::OnStreamEnd() {}

void VideoWidget::OnStreamClose()
{
    play_state_ = kStop;
    render_timer_->stop();

    SAFE_FREE(video_player_);
}

void VideoWidget::OnStreamError()
{
    OnStreamClose();

    QMessageBox::warning(this, tr("Warning"), tr("An error occurred during playback."));
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
