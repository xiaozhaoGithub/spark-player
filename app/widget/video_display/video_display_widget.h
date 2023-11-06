#ifndef VIDEO_DISPLAY_WIDGET_H_
#define VIDEO_DISPLAY_WIDGET_H_

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "widget/video_display/qpainter_render/video_display.h"

class VideoDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoDisplayWidget(QWidget* parent = nullptr);
    ~VideoDisplayWidget();

private slots:
    void PlayClicked();
    void PauseClicked();
    void StopClicked();
    void SelectFileClicked();

    void PlayState(bool playing);

private:
    QLineEdit* file_edit_;
    QPushButton* play_btn_;
    QPushButton* pause_btn_;
    QStackedWidget* play_stop_widget_;
    VideoPlayerWidget* player_;
};

#endif
