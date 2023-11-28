#include "custom_dialog.h"

#include <QHBoxLayout>
#include <QPushButton>

CustomDialog::CustomDialog(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    auto title_bar = new QWidget(this);
    main_widget_ = new QWidget(this);

    auto main_layout = new QVBoxLayout(this);
    main_layout->addWidget(title_bar);
    main_layout->addWidget(main_widget_);
}

ConfirmDialog::ConfirmDialog(QWidget* parent)
    : CustomDialog(parent)
{
    auto ok_btn = new QPushButton(tr("OK"), this);
    connect(ok_btn, &QPushButton::clicked, this, &ConfirmDialog::OkClicked);

    auto cancel_btn = new QPushButton(tr("Cancel"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &ConfirmDialog::CancelClicked);

    auto bottom_layout = new QHBoxLayout;
    bottom_layout->setSpacing(12);
    bottom_layout->addStretch();
    bottom_layout->addWidget(ok_btn);
    bottom_layout->addWidget(cancel_btn);

    AddBottomLayout(bottom_layout);
}

ConfirmDialog::~ConfirmDialog() {}

void ConfirmDialog::OkClicked()
{
    accept();
}
void ConfirmDialog::CancelClicked()
{
    reject();
}

void CustomDialog::AddBottomLayout(QBoxLayout* layout)
{
    auto main_layout = static_cast<QBoxLayout*>(this->layout());
    main_layout->addLayout(layout);
}
