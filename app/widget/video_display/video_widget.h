#ifndef VIDEO_WIDGET_H_
#define VIDEO_WIDGET_H_

#include <QContextMenuEvent>

#include "common/media_info.h"
#include "media_play/ffmpeg/ff_videoplayer.h"
#include "media_play/stream_event_type.h"
#include "render/opengl/render_wnd_gl.h"

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);

    void Open(const MediaInfo& media);
    void Pause();
    void Stop();
    void StartRecord();
    void StopRecord();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void InitUi();
    void InitMenu();

    void Start();

    // event cb
    void StreamEventCallback(StreamEventType type);
    void OnOpenStreamSuccess();
    void OnOpenStreamFail();
    void OnStreamEnd();
    void OnStreamClose();
    void OnStreamError();

private slots:
    void OnEventProcess(StreamEventType type);
    void OnRender();
    void FullScreenClicked();
    void ExitFullScreenClicked();

private:
    QMenu* menu_;

    // decode
    VideoPlayer* video_player_;
    MediaInfo media_;

    // render
    QTimer* render_timer_;
    RenderWnd* render_wnd_;
    int fps_;
};

#endif
