#ifndef RENDER_WND_GL_H_
#define RENDER_WND_GL_H_

#include <QContextMenuEvent>
#include <QMenu>
#include <QOpenGLFunctions>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTimer>
#include <QWidget>
#include <memory>
#include <mutex>

#include "opengl_renderer.h"
#include "common/media_info.h"
#include "media_play/ffmpeg/ff_videoplayer.h"
#include "render/render_wnd.h"

using QOpenGLTexturePtr = std::shared_ptr<QOpenGLTexture>;

class RenderWndGL : public RenderWnd, public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit RenderWndGL(QWidget* parent = nullptr);
    ~RenderWndGL();

    void Open();
    void Pause();
    void Stop();
    void StartRecord();
    void StopRecord();

    void set_media(const MediaInfo& media);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void InitUi();
    void InitMenu();
    void ReallocTex(QOpenGLTexturePtr tex, int type, int width, int height = 1, int depth = 1);
    void ResetTexYuv(const DecodeFrame& frame, int type, int width, int height);
    void ResetTexNV12(const DecodeFrame& frame);
    void FreeTexYuv();
    void FreeTexNV12();

private slots:
    void OnRender();
    void ProcessFrame(const DecodeFrame& frame);
    void FullScreenClicked();
    void ExitFullScreenClicked();

private:
    VideoPlayer* video_player_;
    QTimer* render_timer_;
    std::shared_ptr<OpenGLRenderer> renderer_;

    std::mutex mutex_;

    QSize frame_size_;
    int format_;
    QOpenGLPixelTransferOptions pix_transfer_opts_;
    std::shared_ptr<QOpenGLTexture> y_tex_;
    std::shared_ptr<QOpenGLTexture> u_tex_;
    std::shared_ptr<QOpenGLTexture> v_tex_;
    std::shared_ptr<QOpenGLTexture> uv_tex_;

    quint32 vao_;

    QMenu* menu_;
};

#endif
