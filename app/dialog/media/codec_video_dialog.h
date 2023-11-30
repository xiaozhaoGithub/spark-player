#ifndef DECODE_VIDEO_DIGLOG_H_
#define DECODE_VIDEO_DIGLOG_H_

#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QTabWidget>
#include <QTimeEdit>

#include "common/media_info.h"
#include "dialog/base/custom_dialog.h"
#include "widget/common/widgets.h"

class CodecVideoDialog : public ConfirmDialog
{
    Q_OBJECT

public:
    explicit CodecVideoDialog(QWidget* parent = nullptr);
    ~CodecVideoDialog();

private:
    enum TabIndex
    {
        kDecode,
        kEncode,
        kTranscode
    };

    void Transcode();
    void Decode();
    void Encode();


private slots:
    void OkClicked() override;

private:
    FolderLineEdit* infile_edit_;
    FolderLineEdit* outfile_edit_;

    QTabWidget* codec_tabwidget_;

    QLineEdit* width_edit_;
    QLineEdit* height_edit_;
    QLineEdit* framerate_edit_;
    QLineEdit* bitrate_edit_;
    QLineEdit* gop_size_edit_;
    QLineEdit* max_b_frames_edit_;

    QComboBox* codec_combo_;
    QComboBox* encoder_speed_combo_;

    QTimeEdit* start_time_edit_;
    QTimeEdit* end_time_edit_;
};

#endif
