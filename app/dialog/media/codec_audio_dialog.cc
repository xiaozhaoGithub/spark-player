#include "codec_audio_dialog.h"

#include <QFileDialog>
#include <QMessagebox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "codec/ffmpeghelper.h"
#include "widget/common/uihelper.h"
#include "widget/common/widgets.h"

CodecAudioDialog::CodecAudioDialog(QWidget* parent)
    : ConfirmDialog(parent)
{
    setWindowTitle(tr("Codec Audio"));

    infile_edit_ = new FolderLineEdit(this);
    outfile_edit_ = new FolderLineEdit(this);

    // normal
    auto normal_title = new QLabel(tr("Normal"), this);
    sample_rate_edit_ = new QLineEdit(this);
    bit_rate_edit_ = new QLineEdit(this);

    channel_combo_ = new QComboBox(this);
    channel_combo_->addItem(tr("mono"), 1);
    channel_combo_->addItem(tr("stereo"), 2);
    channel_combo_->addItem(tr("surround"), 3);

    QList<QPair<QWidget*, QWidget*>> widget_pair_list;
    widget_pair_list.append(qMakePair(new QLabel(tr("Sample Rate:")), sample_rate_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Bit Rate:")), bit_rate_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Channel:")), channel_combo_));
    auto normal_layout = uihelper::InitDialogGridLayout(widget_pair_list);

    // codec
    auto codec_title = new QLabel(tr("Codec"), this);
    auto codec_tip = new IconButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), this);
    codec_tip->setFixedSize(16, 16);
    codec_tip->setToolTip(tr("Only pcm files in f32le format are supported"));

    codec_combo_ = new QComboBox(this);
    codec_combo_->addItem(tr("mp3"), 0);
    codec_combo_->addItem(tr("aac"), 1);

    widget_pair_list.clear();
    widget_pair_list.append(qMakePair(new QLabel(tr("Codec:")), codec_combo_));
    auto codec_layout = uihelper::InitDialogGridLayout(widget_pair_list);

    codec_tabwidget_ = new QTabWidget(this);
    auto encoder_widget = new QWidget(codec_tabwidget_);

    codec_tabwidget_->insertTab(kDecode, new QWidget(codec_tabwidget_), tr("Decode"));
    codec_tabwidget_->insertTab(kEncode, encoder_widget, tr("Encode"));

    auto codec_title_layout = new QHBoxLayout;
    codec_title_layout->addWidget(codec_title);
    codec_title_layout->addWidget(codec_tip);
    codec_title_layout->addStretch();

    auto encode_layout = new QVBoxLayout(encoder_widget);
    encode_layout->addWidget(normal_title);
    encode_layout->addLayout(normal_layout);
    encode_layout->addLayout(codec_title_layout);
    encode_layout->addLayout(codec_layout);
    encode_layout->setAlignment(normal_title, Qt::AlignLeft);
    encode_layout->setAlignment(codec_title, Qt::AlignLeft);

    auto main_layout = new QVBoxLayout(main_widget_);
    main_layout->addWidget(infile_edit_);
    main_layout->addWidget(outfile_edit_);
    main_layout->addWidget(codec_tabwidget_);
}

CodecAudioDialog::~CodecAudioDialog() {}

void CodecAudioDialog::Encode()
{
    FFmpegHelper::AudioInfo info;
    info.codec_id = codec_combo_->currentData().toInt();
    info.bit_rate = bit_rate_edit_->text().toInt();
    info.sample_rate = sample_rate_edit_->text().toInt();
    info.channels = channel_combo_->currentData().toInt();

    bool ret = FFmpegHelper::SaveEncodeAudio(info, infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());
    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded audio file"));
    } else {
        QMessageBox::warning(this, tr("warning"), tr("Failed to save the encoded audio file"));
    }
}

void CodecAudioDialog::Decode()
{
    bool ret = FFmpegHelper::SaveDecodeAudio(infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());
    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded audio file"));
    } else {
        QMessageBox::warning(this, tr("warning"), tr("Failed to save the decoded audio file"));
    }
}

void CodecAudioDialog::OkClicked()
{
    int index = codec_tabwidget_->currentIndex();

    switch (index) {
    case CodecAudioDialog::kDecode:
        Decode();
        break;
    case CodecAudioDialog::kEncode:
        Encode();
        break;
    default:
        break;
    }
}
