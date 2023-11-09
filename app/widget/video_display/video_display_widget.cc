#include "video_display_widget.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QRadioButton>

#include "common/singleton.h"
#include "config/config.h"
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

    auto software_dc = new QRadioButton(tr("soft decoding"), this);
    auto hardware_dc = new QRadioButton(tr("hard decoding"), this);

    bool enable_hw_decode =
        Singleton<Config>::Instance()->AppConfigData("video_param", "enable_hw_decode", false).toBool();
    software_dc->setChecked(enable_hw_decode ? false : true);
    hardware_dc->setChecked(enable_hw_decode ? true : false);

    auto dc_btn_group = new QButtonGroup(this);
    dc_btn_group->addButton(software_dc, 0);
    dc_btn_group->addButton(hardware_dc, 1);
    connect(dc_btn_group, qOverload<int>(&QButtonGroup::buttonClicked), this,
            &VideoDisplayWidget::DecodeBtnClicked);

    auto btn_layout = new QHBoxLayout;
    btn_layout->addWidget(file_edit_);
    btn_layout->addWidget(select_file_btn);
    btn_layout->addStretch(9);
    btn_layout->addWidget(software_dc);
    btn_layout->addWidget(hardware_dc);
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

void VideoDisplayWidget::DecodeBtnClicked(int id)
{
    Singleton<Config>::Instance()->SetAppConfigData("video_param", "enable_hw_decode", (bool)id);
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
