#ifndef OPEN_MEDIA_DIALOG_H_
#define OPEN_MEDIA_DIALOG_H_

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

#include "common/media_info.h"

class OpenMediaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenMediaDialog(QWidget* parent = nullptr);
    ~OpenMediaDialog();

    MediaInfo media();

private slots:
    void SelectFileClicked();
    void OpenClicked();
    void CancelClicked();

private:
    QTabWidget* tabwidget_;
    QLineEdit* file_edit_;
    QLineEdit* url_edit_;
    QComboBox* capture_dev_combo_;
};

#endif
