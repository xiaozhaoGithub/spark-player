#include "widgets.h"

#include <QFileDialog>
#include <QPainter>

IconButton::IconButton(QWidget* parent)
    : QLabel(parent)
{}

IconButton::IconButton(const QIcon& icon, QWidget* parent)
    : IconButton(parent)
{
    setPixmap(icon.pixmap(16, 16));
}

void IconButton::mousePressEvent(QMouseEvent* ev)
{
    emit clicked();
}

ComboLineEdit::ComboLineEdit(QWidget* parent)
    : QLineEdit(parent)
    , spacing_(10)
{
    right_layout_ = new QHBoxLayout;

    auto main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);

    main_layout->addStretch();
    main_layout->addLayout(right_layout_);
}

void ComboLineEdit::AddRightWidget(QWidget* widget)
{
    right_layout_->addWidget(widget);
    right_layout_->addSpacing(spacing_);
}

IconButton* ComboLineEdit::CreateStandardIcon(QStyle::StandardPixmap standardIcon)
{
    auto icon = style()->standardIcon(standardIcon);
    auto btn = new IconButton(icon, this);

    int w = right_layout_->sizeHint().width();

    AddRightWidget(btn);
    AdjustPadding();

    return btn;
}

void ComboLineEdit::AdjustPadding()
{
    int w = right_layout_->sizeHint().width();
    setTextMargins(spacing_, textMargins().top(), w + spacing_, textMargins().bottom());
}

FolderLineEdit::FolderLineEdit(QWidget* parent)
    : ComboLineEdit(parent)
{
    setFixedWidth(360);

    auto btn = CreateStandardIcon(QStyle::SP_DialogOpenButton);
    connect(btn, &IconButton::clicked, this, &FolderLineEdit::SelectFileClicked);
}

void FolderLineEdit::SelectFileClicked()
{
    if (select_file_handle_) {
        select_file_handle_();
    } else {
        QFileDialog dlg;
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        auto files = dlg.selectedFiles();
        setText(files.at(0));
        setToolTip(files.at(0));
    }
}
