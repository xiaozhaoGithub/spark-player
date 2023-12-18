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
    , recording_(false)
{
    qRegisterMetaType<StreamEventType>("StreamEventType");

    InitUi();
}

VideoWidget::~VideoWidget()
{
    Stop();
}

void VideoWidget::Open(const MediaInfo& media)
{
    Start(media);
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

    render_timer_->stop();

    StopRecording();

    play_state_ = kStop;
    video_player_->Stop();

    SAFE_FREE(video_player_);
}

void VideoWidget::resizeEvent(QResizeEvent* event)
{
    render_wnd_->setGeometry(rect());
}

void VideoWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);

    menu_->ShowMenu(mapToGlobal(event->pos()));
}

void VideoWidget::InitUi()
{
    menu_ = new VideoMenu(this);
    connect(menu_, &VideoMenu::FullScreen, this, &VideoWidget::FullScreen);
    connect(menu_, &VideoMenu::ExitFullScreen, this, &VideoWidget::ExitFullScreen);
    connect(menu_, &VideoMenu::StartRecording, this, &VideoWidget::StartRecording);
    connect(menu_, &VideoMenu::StopRecording, this, &VideoWidget::StopRecording);

    int type = Singleton<Config>::Instance()->AppConfigData("video_param", "render_type").toInt();
    render_wnd_ = RenderFactory::Create(static_cast<RenderType>(type), this);

    fps_ = Singleton<Config>::Instance()->AppConfigData("video_param", "fps").toInt();

    render_timer_ = new QTimer(this);
    render_timer_->setTimerType(Qt::PreciseTimer);
    connect(render_timer_, &QTimer::timeout, this, &VideoWidget::OnRender);
}

void VideoWidget::Start(const MediaInfo& media)
{
    if (!video_player_) {
        video_player_ = VideoPlayerFactory::Create(media.type);
        video_player_->set_media(media);
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

    render_timer_->start(1000 / (fps_ ? fps_ : video_player_->fps()));
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

    render_timer_->start(1000 / (fps_ ? fps_ : video_player_->fps()));
}

void VideoWidget::OnOpenStreamFail()
{
    OnStreamClose();

    QMessageBox::warning(this, tr("Warning"), tr("Failed to open stream"));
}

void VideoWidget::OnStreamEnd() {}

void VideoWidget::OnStreamClose()
{
    Stop();

    render_wnd_->update();

    emit StreamClosed();
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

void VideoWidget::FullScreen()
{
    if (isFullScreen())
        return;

    setWindowFlags(Qt::Window);

    // Direct full screen will only be full screen on the main screen and placed in the Qt queue.
    QMetaObject::invokeMethod(this, "showFullScreen", Qt::QueuedConnection);

    menu_->SetFullscreenState(true);
}

void VideoWidget::ExitFullScreen()
{
    if (!isFullScreen())
        return;

    setWindowFlags(Qt::Widget);
    showNormal();

    menu_->SetFullscreenState(false);
}

void VideoWidget::StartRecording()
{
    if (!video_player_)
        return;

    QString filename =
        QApplication::applicationDirPath() + "/"
        + QString("record_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    MediaInfo media_info = video_player_->media();
    media_info.src = filename.toStdString();

    video_player_->StartRecord(filename.toStdString().c_str());

    recording_ = true;
    menu_->SetRecordState(recording_);
}

void VideoWidget::StopRecording()
{
    video_player_->StopRecord();

    recording_ = false;
    menu_->SetRecordState(recording_);
}
