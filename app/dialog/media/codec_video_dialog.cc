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
    : ConfirmDialog(parent)
{
    setWindowTitle(tr("Codec Video"));

    infile_edit_ = new FolderLineEdit(this);
    outfile_edit_ = new FolderLineEdit(this);

    // normal
    auto normal_title = new QLabel(tr("Normal"), this);
    width_edit_ = new QLineEdit(this);
    height_edit_ = new QLineEdit(this);
    framerate_edit_ = new QLineEdit(this);
    bitrate_edit_ = new QLineEdit(this);
    gop_size_edit_ = new QLineEdit(this);
    max_b_frames_edit_ = new QLineEdit(this);

    QList<QPair<QWidget*, QWidget*>> widget_pair_list;
    widget_pair_list.append(qMakePair(new QLabel(tr("Width:")), width_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Height:")), height_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Frame Rate:")), framerate_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Bit Rate:")), bitrate_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("I Frame Interval:")), gop_size_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Max B Frames:")), max_b_frames_edit_));

    auto normal_grid_layout = InitGridLayout(widget_pair_list);

    // codec
    auto codec_title = new QLabel(tr("Codec"), this);
    codec_combo_ = new QComboBox(this);
    codec_combo_->addItem(tr("H.264"), "libx264");
    codec_combo_->addItem(tr("H.265"), "libx265");

    encoder_speed_combo_ = new QComboBox(this);
    encoder_speed_combo_->addItem(tr("Slow"));

    widget_pair_list.clear();
    widget_pair_list.append(qMakePair(new QLabel(tr("Codec:")), codec_combo_));
    widget_pair_list.append(qMakePair(new QLabel(tr("Encode Speed:")), encoder_speed_combo_));
    auto codec_grid_layout = InitGridLayout(widget_pair_list);

    // crop
    auto crop_title = new QLabel(tr("Crop"), this);
    start_time_edit_ = new QTimeEdit(this);
    start_time_edit_->setDisplayFormat("hh:mm:ss");
    start_time_edit_->setMinimumTime(QTime(0, 0, 0));
    start_time_edit_->setMaximumTime(QTime(23, 59, 59));
    start_time_edit_->setTime(QTime(0, 0, 0));

    end_time_edit_ = new QTimeEdit(this);
    end_time_edit_->setDisplayFormat("hh:mm:ss");
    end_time_edit_->setMinimumTime(QTime(0, 0, 0));
    end_time_edit_->setMaximumTime(QTime(23, 59, 59));
    end_time_edit_->setTime(QTime(0, 0, 0));

    widget_pair_list.clear();
    widget_pair_list.append(qMakePair(new QLabel(tr("Start Time:")), start_time_edit_));
    widget_pair_list.append(qMakePair(new QLabel(tr("End Time:")), end_time_edit_));
    auto crop_grid_layout = InitGridLayout(widget_pair_list);

    codec_tabwidget_ = new QTabWidget(this);
    auto encoder_widget = new QWidget(codec_tabwidget_);
    auto transcode_widget = new QWidget(codec_tabwidget_);

    codec_tabwidget_->insertTab(kDecode, new QWidget(codec_tabwidget_), tr("Decode"));
    codec_tabwidget_->insertTab(kEncode, encoder_widget, tr("Encode"));
    codec_tabwidget_->insertTab(kTranscode, transcode_widget, tr("Transcode"));

    auto param_layout = new QVBoxLayout(encoder_widget);
    param_layout->addWidget(normal_title);
    param_layout->addLayout(normal_grid_layout);
    param_layout->addWidget(codec_title);
    param_layout->addLayout(codec_grid_layout);
    param_layout->setAlignment(normal_title, Qt::AlignLeft);
    param_layout->setAlignment(codec_title, Qt::AlignLeft);

    auto transcode_layout = new QVBoxLayout(transcode_widget);
    transcode_layout->addWidget(crop_title);
    transcode_layout->addLayout(crop_grid_layout);
    transcode_layout->addStretch();

    auto main_layout = new QVBoxLayout(main_widget_);
    main_layout->addWidget(infile_edit_);
    main_layout->addWidget(outfile_edit_);
    main_layout->addWidget(codec_tabwidget_);
}

CodecVideoDialog::~CodecVideoDialog() {}

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

    bool ret = FFmpegHelper::SaveEncodeVideo(info, infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the encoded video file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the encoded video file"));
    }
}

QGridLayout* CodecVideoDialog::InitGridLayout(const QList<QPair<QWidget*, QWidget*>>& widget_pair_list)
{
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
}

void CodecVideoDialog::Transcode()
{
    FFmpegHelper::AvInfo info;
    info.start_time = start_time_edit_->time().msecsSinceStartOfDay() / 1000;
    info.end_time = end_time_edit_->time().msecsSinceStartOfDay() / 1000;

    bool ret = FFmpegHelper::SaveTranscodeFormat(info, infile_edit_->text().toStdString().data(),
                                                 outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the transcode file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the transcode file"));
    }
}

void CodecVideoDialog::Decode()
{
    bool ret = FFmpegHelper::SaveDecodeVideo(infile_edit_->text().toStdString().data(),
                                             outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded video file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded video file"));
    }
}


void CodecVideoDialog::OkClicked()
{

    bool ret = FFmpegHelper::ExportSingleStream(0, infile_edit_->text().toStdString().data(),
                                                outfile_edit_->text().toStdString().data());

    if (ret) {
        QMessageBox::information(this, tr("Info"), tr("Successfully save the decoded video file"));
    } else {
        QMessageBox::warning(this, tr("Info"), tr("Failed to save the decoded video file"));
    }

    int index = codec_tabwidget_->currentIndex();

    switch (index) {
    case CodecVideoDialog::kDecode:
        Decode();
        break;
    case CodecVideoDialog::kEncode:
        Encode();
        break;
    case CodecVideoDialog::kTranscode:
        Transcode();
        break;
    default:
        break;
    }
}

void CodecVideoDialog::CancelClicked()
{
    reject();
}
