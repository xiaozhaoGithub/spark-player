#ifndef VIDEO_WIDGET_H_
#define VIDEO_WIDGET_H_

#include <QContextMenuEvent>

#include "common/media_info.h"
#include "media_play/ffmpeg/ff_videoplayer.h"
#include "render/opengl/render_wnd_gl.h"

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);

    void Open();
    void Pause();
    void Stop();
    void StartRecord();
    void StopRecord();

    void set_media(const MediaInfo& media);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void InitUi();
    void InitMenu();

private slots:
    void OnRender();
    void FullScreenClicked();
    void ExitFullScreenClicked();

private:
    QMenu* menu_;

    // decode
    VideoPlayer* video_player_;

    // render
    QTimer* render_timer_;
    RenderWnd* render_wnd_;
};

#endif
