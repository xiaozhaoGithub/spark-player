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

#include "render/opengl/video_surface_gl.h"
#include "render/qpainter/video_player_widget.h"

class VideoDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoDisplayWidget(QWidget* parent = nullptr);
    ~VideoDisplayWidget();

private slots:
    void SelectFileClicked();
    void PlayClicked();
    void PauseClicked();
    void StopClicked();
    void DecodeBtnClicked(int id);

    void PlayState(bool playing);

private:
    // VideoPlayerWidget* player_;
    VideoSurfaceGL* player_;

    QLineEdit* file_edit_;
    QPushButton* play_btn_;
    QPushButton* pause_btn_;
    QStackedWidget* play_stop_widget_;
};

#endif
