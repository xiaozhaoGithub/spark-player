#ifndef CUSTOM_DIALOG_H_
#define CUSTOM_DIALOG_H_

#include <QDialog>
#include <QVBoxLayout>

class CustomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomDialog(QWidget* parent, Qt::WindowFlags f = Qt::WindowFlags());
    ~CustomDialog() = default;

    void AddBottomLayout(QBoxLayout* layout);

protected:
    QWidget* main_widget_;
};

class ConfirmDialog : public CustomDialog
{
    Q_OBJECT

public:
    explicit ConfirmDialog(QWidget* parent = nullptr);
    ~ConfirmDialog();

protected slots:
    virtual void OkClicked();
    virtual void CancelClicked();

private:
};

#endif
