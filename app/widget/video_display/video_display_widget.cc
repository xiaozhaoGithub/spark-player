#include "video_display_widget.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QRadioButton>

#include "common/singleton.h"
#include "config/config.h"
#include "dialog/media/open_media_dialog.h"
#include "media_play/ffmpeg/ff_videoplayer.h"

VideoDisplayWidget::VideoDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(1024, 768);

    file_edit_ = new FolderLineEdit(this);
    file_edit_->setReadOnly(true);
    file_edit_->set_select_file_handle(std::bind(&VideoDisplayWidget::SelectMediaClicked, this));

    // player_ = new VideoPlayerWidget(this);
    // connect(player_, &VideoPlayerWidget::PlayState, this, &VideoDisplayWidget::PlayState);

    video_widget_ = new VideoWidget(this);
    // connect(player_, &RenderWndGL::PlayState, this, &VideoDisplayWidget::PlayState);
    // connect(player_, &RenderWndGL::RecordState, this, &VideoDisplayWidget::RecordState);

    auto fill_bg_wiget = new QWidget(this);

    play_btn_ = new QPushButton(tr("Play"), this);
    play_btn_->setFixedHeight(32);

    pause_btn_ = new QPushButton(tr("Pause"), this);
    pause_btn_->setFixedHeight(32);

    auto stop_btn = new QPushButton(tr("Stop"), this);
    stop_btn->setFixedHeight(32);

    play_stop_widget_ = new QStackedWidget(this);
    play_stop_widget_->addWidget(play_btn_);
    play_stop_widget_->addWidget(pause_btn_);
    play_stop_widget_->setCurrentWidget(play_btn_);
    play_stop_widget_->setFixedHeight(32);

    record_btn_ = new QPushButton(tr("Record"), this);
    record_btn_->setFixedHeight(32);

    stop_record_btn_ = new QPushButton(tr("Stop Record"), this);
    stop_record_btn_->setFixedHeight(32);

    record_widget_ = new QStackedWidget(this);
    record_widget_->addWidget(record_btn_);
    record_widget_->addWidget(stop_record_btn_);
    record_widget_->setCurrentWidget(record_btn_);
    record_widget_->setFixedHeight(32);

    connect(play_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::PlayClicked);
    connect(pause_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::PauseClicked);
    connect(stop_btn, &QPushButton::clicked, this, &VideoDisplayWidget::StopClicked);
    connect(record_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::RecordClicked);
    connect(stop_record_btn_, &QPushButton::clicked, this, &VideoDisplayWidget::StopRecordClicked);

    auto software_dc = new QRadioButton(tr("Soft Decoding"), this);
    auto hardware_dc = new QRadioButton(tr("Hard Decoding"), this);

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
    btn_layout->addStretch(9);
    btn_layout->addWidget(software_dc);
    btn_layout->addWidget(hardware_dc);
    btn_layout->addWidget(play_stop_widget_);
    btn_layout->addWidget(stop_btn);
    btn_layout->addWidget(record_widget_);

    auto main_layout = new QVBoxLayout(this);
    main_layout->addWidget(video_widget_, 9);
    main_layout->addWidget(fill_bg_wiget);
    main_layout->addLayout(btn_layout);
}

VideoDisplayWidget::~VideoDisplayWidget() {}

void VideoDisplayWidget::PlayClicked()
{
    video_widget_->set_media(media_);
    video_widget_->Open();
}

void VideoDisplayWidget::PauseClicked()
{
    play_stop_widget_->setCurrentWidget(play_btn_);
    video_widget_->Pause();
}

void VideoDisplayWidget::StopClicked()
{
    video_widget_->Stop();
}

void VideoDisplayWidget::RecordClicked()
{
    video_widget_->StartRecord();
}

void VideoDisplayWidget::StopRecordClicked()
{
    video_widget_->StopRecord();
}

void VideoDisplayWidget::DecodeBtnClicked(int id)
{
    Singleton<Config>::Instance()->SetAppConfigData("video_param", "enable_hw_decode", (bool)id);
}

void VideoDisplayWidget::PlayState(bool playing)
{
    play_stop_widget_->setCurrentWidget(playing ? pause_btn_ : play_btn_);
}

void VideoDisplayWidget::RecordState(bool recording)
{
    record_widget_->setCurrentWidget(recording ? stop_record_btn_ : record_btn_);
}

void VideoDisplayWidget::SelectMediaClicked()
{
    auto dlg = new OpenMediaDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &OpenMediaDialog::finished, this, [=](int code) {
        if (code == QDialog::Accepted) {
            media_ = dlg->media();
            file_edit_->setText(QString::fromStdString(media_.src));
            file_edit_->setToolTip(QString::fromStdString(media_.src));
        }
    });

    dlg->open();
}
