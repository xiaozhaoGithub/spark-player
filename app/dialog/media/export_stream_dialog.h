#ifndef EXPORT_STREAM_DIALOG_H_
#define EXPORT_STREAM_DIALOG_H_

#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include "dialog/base/custom_dialog.h"

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
    QLineEdit* file_edit_;
    QPushButton* select_file_btn_;

    QButtonGroup* btn_group_;
};

#endif
