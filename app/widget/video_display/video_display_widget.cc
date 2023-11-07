#include "video_display_widget.h"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>

#include "render/video_renderer.h"

VideoDisplayWidget::VideoDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(1024, 768);

    file_edit_ = new QLineEdit(this);
    file_edit_->setFixedWidth(360);
    auto select_file_btn = new QPushButton(tr("select"), this);
    connect(select_file_btn, &QPushButton::clicked, this, &VideoDisplayWidget::SelectFileClicked);

    player_ = new VideoPlayerWidget(this);
    connect(player_, &VideoPlayerWidget::PlayState, this, &VideoDisplayWidget::PlayState);

    auto fill_bg_wiget = new QWidget(this);

    play_btn_ = new QPushButton(tr("play"), this);
    play_btn_->setFixedHeight(32);

    pause_btn_ = new QPushButton(tr("pause"), this);
    pause_btn_->setFixedHeight(32);

    auto stop_btn = new QPushButton(tr("stop"), this);
    stop_btn->setFixedHeight(32);

    play_stop_widget_ = new QStackedWidget(this);
    play_stop_widget_->addWidget(play_btn_);
    play_stop_widget_->addWidget(pause_btn_);
    play_stop_widget_->setCurrentWidget(play_btn_);
    play_stop_widget_->setFixedHeight(32);

    connect(play_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::PlayClicked);
    connect(pause_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::PauseClicked);
    connect(stop_btn, &QPushButton::clicked, this, &VideoDisplayWidget::StopClicked);

    auto btn_layout = new QHBoxLayout;
    btn_layout->addWidget(file_edit_);
    btn_layout->addWidget(select_file_btn);
    btn_layout->addStretch(9);
    btn_layout->addWidget(play_stop_widget_);
    btn_layout->addWidget(stop_btn);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(player_, 9);
    mainLayout->addWidget(fill_bg_wiget);
    mainLayout->addLayout(btn_layout);
}

VideoDisplayWidget::~VideoDisplayWidget() {}

void VideoDisplayWidget::PlayClicked()
{
    play_stop_widget_->setCurrentWidget(pause_btn_);

    auto filename = file_edit_->text().toUtf8();
    player_->Open(filename);
}

void VideoDisplayWidget::PauseClicked()
{
    play_stop_widget_->setCurrentWidget(play_btn_);
    player_->Pause();
}

void VideoDisplayWidget::StopClicked()
{
    player_->Stop();
}

void VideoDisplayWidget::SelectFileClicked()
{
    QFileDialog dlg;
    int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        file_edit_->setText(files.at(0));
        file_edit_->setToolTip(files.at(0));
    }
}

void VideoDisplayWidget::PlayState(bool playing)
{
    play_stop_widget_->setCurrentWidget(playing ? pause_btn_ : play_btn_);
}
