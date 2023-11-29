#ifndef EXPORT_STREAM_DIALOG_H_
#define EXPORT_STREAM_DIALOG_H_

#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "dialog/base/custom_dialog.h"
#include "widget/common/widgets.h"

class ExportStreamDialog : public ConfirmDialog
{
    Q_OBJECT

public:
    explicit ExportStreamDialog(QWidget* parent = nullptr);

private slots:
    void SelectFileClicked();

    void OkClicked() override;
    void CancelClicked() override;

private:
    FolderLineEdit* file_edit_;
    QButtonGroup* btn_group_;
};

#endif
