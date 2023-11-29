#ifndef OPEN_MEDIA_DIALOG_H_
#define OPEN_MEDIA_DIALOG_H_

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

#include "common/media_info.h"
#include "dialog/base/custom_dialog.h"
#include "widget/common/widgets.h"

class OpenMediaDialog : public ConfirmDialog
{
    Q_OBJECT

public:
    explicit OpenMediaDialog(QWidget* parent = nullptr);
    ~OpenMediaDialog();

    MediaInfo media();

private:
    QTabWidget* tabwidget_;

    FolderLineEdit* file_edit_;
    QLineEdit* url_edit_;
    QComboBox* capture_dev_combo_;
};

#endif
