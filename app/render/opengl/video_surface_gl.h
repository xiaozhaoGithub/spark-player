#ifndef VIDEO_SURFACE_GL_H_
#define VIDEO_SURFACE_GL_H_

#include <QContextMenuEvent>
#include <QMenu>
#include <QOpenGLFunctions>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QWidget>
#include <memory>
#include <mutex>

#include "opengl_renderer.h"
#include "codec/video_codec_manager.h"

class VideoSurfaceGL : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoSurfaceGL(QWidget* parent = nullptr);
    ~VideoSurfaceGL();

    void Open(const char* name);
    void Pause();
    void Stop();

signals:
    void PlayState(bool playing);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void InitMenu();

private slots:
    void ProcessFrame(AVFrame* frame);
    void FullScreenClicked();
    void ExitFullScreenClicked();

private:
    VideoCodecManager* video_renderer_;
    std::shared_ptr<OpenGLRenderer> renderer_;

    std::mutex mutex_;

    QSize frame_size_;
    QOpenGLPixelTransferOptions pix_transfer_opts_;
    std::shared_ptr<QOpenGLTexture> y_tex_;
    std::shared_ptr<QOpenGLTexture> u_tex_;
    std::shared_ptr<QOpenGLTexture> v_tex_;

    quint32 vao_;

    QMenu* menu_;
};

#endif
