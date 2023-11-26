#include "codec_video_dialog.h"

#include <QFileDialog>
#include <QLabel>
#include <QList>
#include <QMessagebox>
#include <QPair>
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

    // normal
    auto normal_title = new QLabel(tr("Normal"), this);
    width_edit_ = new QLineEdit(this);
    height_edit_ = new QLineEdit(this);
    framerate_edit_ = new QLineEdit(this);
    bitrate_edit_ = new QLineEdit(this);
    gop_size_edit_ = new QLineEdit(this);
    max_b_frames_edit_ = new QLineEdit(this);

    QList<QPair<QWidget*, QWidget*>> normal_pair_list;
    normal_pair_list.append(qMakePair(new QLabel(tr("Width:")), width_edit_));
    normal_pair_list.append(qMakePair(new QLabel(tr("Height:")), height_edit_));
    normal_pair_list.append(qMakePair(new QLabel(tr("Frame Rate:")), framerate_edit_));
    normal_pair_list.append(qMakePair(new QLabel(tr("Bit Rate:")), bitrate_edit_));
    normal_pair_list.append(qMakePair(new QLabel(tr("I Frame Interval:")), gop_size_edit_));
    normal_pair_list.append(qMakePair(new QLabel(tr("Max B Frames:")), max_b_frames_edit_));

    // codec
    auto codec_title = new QLabel(tr("Codec"), this);
    codec_combo_ = new QComboBox(this);
    codec_combo_->addItem(tr("H.264"), "libx264");
    codec_combo_->addItem(tr("H.265"), "libx265");

    encoder_speed_combo_ = new QComboBox(this);
    encoder_speed_combo_->addItem(tr("Slow"));

    QList<QPair<QWidget*, QWidget*>> codec_pair_list;
    codec_pair_list.append(qMakePair(new QLabel(tr("Codec:")), codec_combo_));
    codec_pair_list.append(qMakePair(new QLabel(tr("Encode Speed:")), encoder_speed_combo_));

    codec_tabwidget_ = new QTabWidget(this);
    auto encoder_info_widget = new QWidget(codec_tabwidget_);
    codec_tabwidget_->addTab(new QWidget(codec_tabwidget_), tr("Decoder"));
    codec_tabwidget_->addTab(encoder_info_widget, tr("Encoder"));
    codec_tabwidget_->setCurrentIndex(0);

    auto ok_btn = new QPushButton(tr("OK"), this);
    connect(ok_btn, &QPushButton::clicked, this, &CodecVideoDialog::OkClicked);

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

    auto InitGridLayout = [](const QList<QPair<QWidget*, QWidget*>>& widget_pair_list) -> QGridLayout* {
        int row = 0;
        int rol = 0;
        auto grid_layout = new QGridLayout;
        grid_layout->setContentsMargins(12, 8, 12, 8);

        for (auto pair : widget_pair_list) {
            rol = 0;
            grid_layout->addWidget(pair.first, row, rol++, Qt::AlignLeft);
            grid_layout->addWidget(pair.second, row++, rol);
        }

        return grid_layout;
    };
    auto normal_grid_layout = InitGridLayout(normal_pair_list);
    auto codec_grid_layout = InitGridLayout(codec_pair_list);

    auto param_layout = new QVBoxLayout(encoder_info_widget);

    param_layout->addWidget(normal_title);
    param_layout->addLayout(normal_grid_layout);
    param_layout->addWidget(codec_title);
    param_layout->addLayout(codec_grid_layout);
    param_layout->setAlignment(normal_title, Qt::AlignLeft);
    param_layout->setAlignment(codec_title, Qt::AlignLeft);

    auto bottom_layout = new QHBoxLayout;
    bottom_layout->addStretch();
    bottom_layout->addWidget(ok_btn);
    bottom_layout->addWidget(cancel_btn);

    auto main_layout = new QVBoxLayout(this);
    main_layout->addLayout(file_layout);
    main_layout->addLayout(outfile_layout);
    main_layout->addWidget(codec_tabwidget_);
    main_layout->addLayout(bottom_layout);
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

void CodecVideoDialog::Encode()
{
    FFmpegHelper::AvInfo info;
    info.codec_name = codec_combo_->currentData().toString().toStdString();
    // info.codec_name = encoder_speed_combo_->currentText().toStdString();
    info.video_size = QString(width_edit_->text() + "x" + height_edit_->text()).toStdString();
    info.framerate = framerate_edit_->text().toInt();
    info.bit_rate = bitrate_edit_->text().toInt();
    info.gop_size = gop_size_edit_->text().toInt();
    info.max_b_frames = max_b_frames_edit_->text().toInt();

    bool ret = FFmpegHelper::SaveEncodeVideo(info, file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded video file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the encoded video file"));
    }
}

void CodecVideoDialog::Decode()
{
    bool ret = FFmpegHelper::SaveDecodeVideo(file_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded video file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded video file"));
    }
}


void CodecVideoDialog::OkClicked()
{
    if (codec_tabwidget_->currentIndex() == 0) {
        Decode();
    } else {
        Encode();
    }
}

void CodecVideoDialog::CancelClicked()
{
    reject();
}
