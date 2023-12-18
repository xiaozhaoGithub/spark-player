#ifndef VIDEO_WIDGET_H_
#define VIDEO_WIDGET_H_

#include <QContextMenuEvent>

#include "video_menu.h"
#include "media_play/ffmpeg/ff_videoplayer.h"
#include "media_play/stream_event_type.h"
#include "render/opengl/render_wnd_gl.h"

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);
    ~VideoWidget();

    void Open(const MediaInfo& media);
    void Pause();
    void Stop();

signals:
    void StreamClosed();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    enum PlayState
    {
        kRunning,
        kPause,
        kStop
    };

    void InitUi();
    void Resume();

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
    void FullScreen();
    void ExitFullScreen();
    void StartRecording();
    void StopRecording();

private:
    VideoMenu* menu_;

    // decode
    VideoPlayer* video_player_;
    PlayState play_state_;

    // render
    QTimer* render_timer_;
    RenderWnd* render_wnd_;
    int fps_;

    // encoder
    bool recording_;
    std::unique_ptr<FFmpegWriter> writer_;
};

#endif
