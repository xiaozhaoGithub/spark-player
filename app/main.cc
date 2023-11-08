#include "widget/video_display/video_display_widget.h"

#include <QApplication>
#include <QTextCodec>

#include "spdlog/spdlog.h"

int main(int argc, char* argv[])
{
    std::unique_ptr<QCoreApplication> a;
    a.reset(new QApplication(argc, argv));

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    VideoDisplayWidget w;
    w.show();

    // Register FFmpeg codecs and filters (deprecated in 4.0+)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
#if LIBAVFILTER_VERSION_INT < AV_VERSION_INT(7, 14, 100)
    // avfilter_register_all();
#endif
    SPDLOG_INFO("version: {0}", avcodec_version());

    return a->exec();
}
