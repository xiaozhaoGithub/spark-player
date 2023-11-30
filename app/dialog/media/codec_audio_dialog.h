#ifndef DECODE_AUDIO_DIGLOG_H_
#define DECODE_AUDIO_DIGLOG_H_

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

#include "common/media_info.h"
#include "dialog/base/custom_dialog.h"
#include "widget/common/widgets.h"

class CodecAudioDialog : public ConfirmDialog
{
    Q_OBJECT
public:
    explicit CodecAudioDialog(QWidget* parent = nullptr);
    ~CodecAudioDialog();

private:
    enum TabIndex
    {
        kDecode,
        kEncode,
    };

    void Decode();
    void Encode();

private slots:
    void OkClicked() override;

private:
    FolderLineEdit* infile_edit_;
    FolderLineEdit* outfile_edit_;

    QTabWidget* codec_tabwidget_;

    QLineEdit* sample_rate_edit_;
    QLineEdit* bit_rate_edit_;
    QComboBox* channel_combo_;
    QComboBox* codec_combo_;
};

#endif
