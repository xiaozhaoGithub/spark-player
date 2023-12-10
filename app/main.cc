#include <QApplication>
#include <QTextCodec>

#include "spdlog/spdlog.h"
#include "window/mainwindow/mainwindow.h"

int main(int argc, char* argv[])
{
    std::unique_ptr<QCoreApplication> a;
    a.reset(new QApplication(argc, argv));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    MainWindow w;
    w.show();

    return a->exec();
}
