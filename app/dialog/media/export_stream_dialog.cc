#include "export_stream_dialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "codec/ffmpeghelper.h"

ExportStreamDialog::ExportStreamDialog(QWidget* parent)
    : ConfirmDialog(parent)
{
    setWindowTitle(tr("Export Stream"));

    file_edit_ = new FolderLineEdit(this);

    btn_group_ = new QButtonGroup(this);
    auto video_btn = new QRadioButton(tr("video"), this);
    auto audio_btn = new QRadioButton(tr("audio"), this);
    auto subtitle_btn = new QRadioButton(tr("subtitile"), this);
    btn_group_->addButton(video_btn, 0);
    btn_group_->addButton(audio_btn, 1);
    btn_group_->addButton(subtitle_btn, 3);

    video_btn->setChecked(true);

    auto stream_type_layout = new QHBoxLayout;
    stream_type_layout->addWidget(video_btn);
    stream_type_layout->addWidget(audio_btn);
    stream_type_layout->addWidget(subtitle_btn);
    stream_type_layout->addStretch();

    auto main_layout = new QVBoxLayout(main_widget_);
    main_layout->addWidget(file_edit_);
    main_layout->addLayout(stream_type_layout);
}

void ExportStreamDialog::SelectFileClicked()
{
    QFileDialog dlg;
    int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        file_edit_->setText(files.at(0));
        file_edit_->setToolTip(files.at(0));
    }
}

void ExportStreamDialog::OkClicked()
{
    if (file_edit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select the input video file!"));
        return;
    }

    QFileDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return;

    auto files = dlg.selectedFiles();
    int media_type = btn_group_->id(btn_group_->checkedButton());

    bool ret = FFmpegHelper::ExportSingleStream(media_type, file_edit_->text().toStdString().data(),
                                                files.at(0).toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully to export the stream"));
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to export the stream"));
    }
}

void ExportStreamDialog::CancelClicked()
{
    reject();
}
