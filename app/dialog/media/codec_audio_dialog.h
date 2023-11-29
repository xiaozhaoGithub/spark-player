#ifndef DECODE_AUDIO_DIGLOG_H_
#define DECODE_AUDIO_DIGLOG_H_

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

#include "common/media_info.h"
#include "dialog/base/custom_dialog.h"
#include "widget/common/widgets.h"

class CodecAudioDialog : public CustomDialog
{
    Q_OBJECT

public:
    explicit CodecAudioDialog(QWidget* parent = nullptr);
    ~CodecAudioDialog();

private slots:
    void DecodeClicked();
    void EncodeClicked();
    void CancelClicked();

private:
    FolderLineEdit* infile_edit_;
    FolderLineEdit* outfile_edit_;
};

#endif
