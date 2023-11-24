#ifndef DECODE_AUDIO_DIGLOG_H_
#define DECODE_AUDIO_DIGLOG_H_

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

#include "common/media_info.h"

class CodecAudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CodecAudioDialog(QWidget* parent = nullptr);
    ~CodecAudioDialog();

    MediaInfo media();

private slots:
    void SelectFileClicked();
    void DecodeClicked();
    void EncodeClicked();
    void CancelClicked();

private:
    QLineEdit* file_edit_;
    QLineEdit* outfile_edit_;
    QPushButton* select_file_btn_;
    QPushButton* select_outfile_btn_;
};

#endif
