#include "widgets.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QPainter>
#include <QVBoxLayout>

SelectFileButton::SelectFileButton(QWidget* parent)
    : QPushButton(parent)
{
    auto select_widget = new QWidget(this);

    icon_label_ = new QLabel(this);
    icon_label_->setText("+");

    text_label_ = new QLabel(this);
    text_label_->setText(tr("Import"));

    tip_label_ = new QLabel(this);
    tip_label_->setText(tr("Video"));

    connect(this, &SelectFileButton::clicked, this, &SelectFileButton::SelectFileClicked);

    auto hbox = new QHBoxLayout();
    hbox->addWidget(icon_label_);
    hbox->addWidget(text_label_);

    auto vbox = new QVBoxLayout(this);
    vbox->addLayout(hbox);
    vbox->addWidget(tip_label_);
}

void SelectFileButton::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    QColor bg_color(31, 31, 31);

    if (this->underMouse()) {
        bg_color = QColor(45, 45, 45);
    }

    /* p.setBrush(bg_color);
     p.drawRect(rect());*/
}

// void SelectFileButton::mousePressEvent(QMouseEvent* event)
//{
//    emit clicked();
//}

void SelectFileButton::SelectFileClicked()
{
    QFileDialog dlg;
    int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
        auto files = dlg.selectedFiles();
        this->setText(files.at(0));
        this->setToolTip(files.at(0));
    }
}
