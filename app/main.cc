#include <QApplication>
#include <QTextCodec>

extern "C"
{
#include "libavformat/avformat.h"
}
#include "spdlog/spdlog.h"
#include "window/mainwindow/mainwindow.h"


int main(int argc, char* argv[])
{
    std::unique_ptr<QCoreApplication> a;
    a.reset(new QApplication(argc, argv));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    MainWindow w;
    w.show();

    // Register FFmpeg codecs and filters (deprecated in 4.0+)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(7, 14, 100)
    // avfilter_register_all();
#endif
    SPDLOG_INFO("codec version: {0}", avcodec_version());

    return a->exec();
}
