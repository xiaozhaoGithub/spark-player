#include "codec_audio_dialog.h"

#include <QFileDialog>
#include <QMessagebox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "codec/ffmpeghelper.h"

CodecAudioDialog::CodecAudioDialog(QWidget* parent)
    : QDialog(parent)
{
    file_edit_ = new QLineEdit(this);
    file_edit_->setText("E:/WorkSpace/GithubCode/ffmpeg-player/build/app/Debug/breakout.pcm");
    file_edit_->setFixedWidth(360);
    select_file_btn_ = new QPushButton(tr("Select"), this);
    connect(select_file_btn_, &QPushButton::clicked, this, &CodecAudioDialog::SelectFileClicked);

    outfile_edit_ = new QLineEdit(this);
    outfile_edit_->setText("E:/WorkSpace/GithubCode/ffmpeg-player/build/app/Debug/breakout.pcm");
    outfile_edit_->setFixedWidth(360);
    select_outfile_btn_ = new QPushButton(tr("Select"), this);
    connect(select_outfile_btn_, &QPushButton::clicked, this, &CodecAudioDialog::SelectFileClicked);

    auto mp3_btn = new QRadioButton("mp3", this);
    mp3_btn->setChecked(true);

    auto encode_btn = new QPushButton(tr("Encode"), this);
    connect(encode_btn, &QPushButton::clicked, this, &CodecAudioDialog::EncodeClicked);

    auto decode_btn = new QPushButton(tr("Decode"), this);
    connect(decode_btn, &QPushButton::clicked, this, &CodecAudioDialog::DecodeClicked);

    auto cancel_btn = new QPushButton(tr("Cancel"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &CodecAudioDialog::CancelClicked);

    auto file_layout = new QHBoxLayout;
    file_layout->addWidget(file_edit_);
    file_layout->addWidget(select_file_btn_);
    file_layout->setAlignment(Qt::AlignCenter);

    auto outfile_layout = new QHBoxLayout;
    outfile_layout->addWidget(outfile_edit_);
    outfile_layout->addWidget(select_outfile_btn_);
    outfile_layout->setAlignment(Qt::AlignCenter);

    auto audio_type_layout = new QHBoxLayout;
    audio_type_layout->addWidget(mp3_btn);
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

CodecAudioDialog::~CodecAudioDialog() {}

void CodecAudioDialog::SelectFileClicked()
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

void CodecAudioDialog::EncodeClicked()
{
    bool ret = FFmpegHelper::SaveEncodeAudio(file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded audio file"));
        accept();
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the encoded audio file"));
    }
}

void CodecAudioDialog::DecodeClicked()
{
    bool ret = FFmpegHelper::SaveDecodeAudio(file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded audio file"));
        accept();
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded audio file"));
    }

    accept();
}


void CodecAudioDialog::CancelClicked()
{
    reject();
}
