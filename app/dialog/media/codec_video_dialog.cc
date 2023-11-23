#include "codec_video_dialog.h"

#include <QFileDialog>
#include <QMessagebox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "codec/ffmpeghelper.h"

CodecVideoDialog::CodecVideoDialog(QWidget* parent)
    : QDialog(parent)
{
    file_edit_ = new QLineEdit(this);
    file_edit_->setFixedWidth(360);
    select_file_btn_ = new QPushButton(tr("Select"), this);
    connect(select_file_btn_, &QPushButton::clicked, this, &CodecVideoDialog::SelectFileClicked);

    outfile_edit_ = new QLineEdit(this);
    outfile_edit_->setFixedWidth(360);
    select_outfile_btn_ = new QPushButton(tr("Select"), this);
    connect(select_outfile_btn_, &QPushButton::clicked, this, &CodecVideoDialog::SelectFileClicked);

    auto encode_btn = new QPushButton(tr("Encode"), this);
    connect(encode_btn, &QPushButton::clicked, this, &CodecVideoDialog::EncodeClicked);

    auto decode_btn = new QPushButton(tr("Decode"), this);
    connect(decode_btn, &QPushButton::clicked, this, &CodecVideoDialog::DecodeClicked);

    auto cancel_btn = new QPushButton(tr("Cancel"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &CodecVideoDialog::CancelClicked);

    auto file_layout = new QHBoxLayout;
    file_layout->addWidget(file_edit_);
    file_layout->addWidget(select_file_btn_);
    file_layout->setAlignment(Qt::AlignCenter);

    auto outfile_layout = new QHBoxLayout;
    outfile_layout->addWidget(outfile_edit_);
    outfile_layout->addWidget(select_outfile_btn_);
    outfile_layout->setAlignment(Qt::AlignCenter);

    auto audio_type_layout = new QHBoxLayout;
    audio_type_layout->addStretch();

    auto bottom_layout = new QHBoxLayout;
    bottom_layout->addStretch();
    bottom_layout->addWidget(encode_btn);
    bottom_layout->addWidget(decode_btn);
    bottom_layout->addWidget(cancel_btn);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(file_layout);
    mainLayout->addLayout(outfile_layout);
    mainLayout->addLayout(audio_type_layout);
    mainLayout->addLayout(bottom_layout);
}

CodecVideoDialog::~CodecVideoDialog() {}

void CodecVideoDialog::SelectFileClicked()
{
    QFileDialog dlg;
    int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        if (sender() == select_file_btn_) {
            file_edit_->setText(files.at(0));
            file_edit_->setToolTip(files.at(0));
        } else {
            outfile_edit_->setText(files.at(0));
            outfile_edit_->setToolTip(files.at(0));
        }
    }
}

void CodecVideoDialog::EncodeClicked()
{
    bool ret = FFmpegHelper::SaveEncodeVideo(file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded video file"));
        accept();
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the encoded video file"));
    }
}

void CodecVideoDialog::DecodeClicked()
{
    bool ret = FFmpegHelper::SaveDecodeVideo(file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded video file"));
        accept();
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded video file"));
    }
}


void CodecVideoDialog::CancelClicked()
{
    reject();
}
