#include "codec_audio_dialog.h"

#include <QFileDialog>
#include <QMessagebox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "codec/ffmpeghelper.h"

CodecAudioDialog::CodecAudioDialog(QWidget* parent)
    : CustomDialog(parent)
{
    setWindowTitle(tr("Codec Audio"));

    infile_edit_ = new FolderLineEdit(this);
    outfile_edit_ = new FolderLineEdit(this);

    auto mp3_btn = new QRadioButton("mp3", this);
    mp3_btn->setChecked(true);

    auto encode_btn = new QPushButton(tr("Encode"), this);
    connect(encode_btn, &QPushButton::clicked, this, &CodecAudioDialog::EncodeClicked);

    auto decode_btn = new QPushButton(tr("Decode"), this);
    connect(decode_btn, &QPushButton::clicked, this, &CodecAudioDialog::DecodeClicked);

    auto cancel_btn = new QPushButton(tr("Cancel"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &CodecAudioDialog::CancelClicked);

    auto bottom_layout = new QHBoxLayout;
    bottom_layout->addStretch();
    bottom_layout->addWidget(encode_btn);
    bottom_layout->addWidget(decode_btn);
    bottom_layout->addWidget(cancel_btn);

    auto main_layout = new QVBoxLayout(main_widget_);
    main_layout->addWidget(infile_edit_);
    main_layout->addWidget(outfile_edit_);
    main_layout->addLayout(bottom_layout);
}

CodecAudioDialog::~CodecAudioDialog() {}

void CodecAudioDialog::EncodeClicked()
{
    bool ret = FFmpegHelper::SaveEncodeAudio(infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());
    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded audio file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the encoded audio file"));
    }
}

void CodecAudioDialog::DecodeClicked()
{
    bool ret = FFmpegHelper::SaveDecodeAudio(infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());
    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded audio file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded audio file"));
    }
}


void CodecAudioDialog::CancelClicked()
{
    reject();
}
