#ifndef WIDGETS_H_
#define WIDGETS_H_

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

class IconButton : public QLabel
{
    Q_OBJECT
public:
    explicit IconButton(QWidget* parent = nullptr);
    explicit IconButton(const QIcon& icon, QWidget* parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* ev) override;
};

class ComboLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ComboLineEdit(QWidget* parent = nullptr);

    void AddRightWidget(QWidget* widget);

    IconButton* CreateStandardIcon(QStyle::StandardPixmap icon);

private:
    void AdjustPadding();

protected:
    QHBoxLayout* right_layout_;

private:
    int spacing_;
};

class FolderLineEdit : public ComboLineEdit
{
    Q_OBJECT
public:
    explicit FolderLineEdit(QWidget* parent = nullptr);

    inline void set_select_file_handle(const std::function<void()>& handle);

private slots:
    void SelectFileClicked();

private:
    std::function<void()> select_file_handle_;
};

void FolderLineEdit::set_select_file_handle(const std::function<void()>& handle)
{
    select_file_handle_ = handle;
}

#endif
