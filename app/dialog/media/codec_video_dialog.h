#ifndef DECODE_VIDEO_DIGLOG_H_
#define DECODE_VIDEO_DIGLOG_H_

#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QTabWidget>
#include <QTimeEdit>

#include "common/media_info.h"

class CodecVideoDialog : public QDialog
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

    QGridLayout* InitGridLayout(const QList<QPair<QWidget*, QWidget*>>& widget_pair_list);

    void Transcode();
    void Decode();
    void Encode();


private slots:
    void SelectFileClicked();
    void OkClicked();
    void CancelClicked();

private:
    QLineEdit* file_edit_;
    QLineEdit* outfile_edit_;
    QPushButton* select_file_btn_;
    QPushButton* select_outfile_btn_;

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
