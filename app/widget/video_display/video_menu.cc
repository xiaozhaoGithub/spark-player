#include "video_menu.h"

VideoMenu::VideoMenu(QWidget* parent)
    : QObject(parent)
{
    menu_ = new QMenu(parent);

    fullscreen_ = menu_->addAction(tr("Full Screen"), this, &VideoMenu::FullScreen);
    exit_fullscreen_ = menu_->addAction(tr("Exit Full Screen"), this, &VideoMenu::ExitFullScreen);

    start_recording_ = menu_->addAction(tr("Start Recording"), this, &VideoMenu::StartRecording);
    stop_recording_ = menu_->addAction(tr("Stop Recording"), this, &VideoMenu::StopRecording);

    SetFullscreenState(false);
    SetRecordState(false);
}

VideoMenu::~VideoMenu() {}

void VideoMenu::ShowMenu(const QPoint& pos)
{
    menu_->popup(pos);
}

void VideoMenu::SetFullscreenState(bool fullscreen)
{
    fullscreen_->setVisible(!fullscreen);
    exit_fullscreen_->setVisible(fullscreen);
}

void VideoMenu::SetRecordState(bool start)
{
    start_recording_->setVisible(!start);
    stop_recording_->setVisible(start);
}
