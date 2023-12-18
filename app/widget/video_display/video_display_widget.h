#ifndef VIDEO_DISPLAY_WIDGET_H_
#define VIDEO_DISPLAY_WIDGET_H_

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>

#include "render/qpainter/video_player_widget.h"
#include "widget/common/widgets.h"
#include "widget/video_display/video_widget.h"

class VideoDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoDisplayWidget(QWidget* parent = nullptr);
    ~VideoDisplayWidget();

private slots:
    void SelectMediaClicked();
    void PlayClicked();
    void PauseClicked();
    void StopClicked();
    void DecodeBtnClicked(int id);

    void PlayStateChanged(bool playing);

private:
    VideoWidget* video_widget_;

    FolderLineEdit* file_edit_;
    QPushButton* play_btn_;
    QPushButton* pause_btn_;
    QStackedWidget* play_stop_widget_;

    MediaInfo media_;
};

#endif
