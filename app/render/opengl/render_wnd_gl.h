#ifndef RENDER_WND_GL_H_
#define RENDER_WND_GL_H_

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
#include "render/render_wnd.h"
#include "util/decode_frame.h"

using QOpenGLTexturePtr = std::shared_ptr<QOpenGLTexture>;

class RenderWndGL : public RenderWnd, public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    explicit RenderWndGL(QWidget* parent = nullptr);
    ~RenderWndGL();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void Render(const DecodeFrame& frame) override;
    void setGeometry(const QRect& rect) override { QOpenGLWidget::setGeometry(rect); }
    void update() override { QOpenGLWidget::update(); }

private:
    void ReallocTex(QOpenGLTexturePtr tex, int type, int width, int height = 1, int depth = 1);
    void ResetTexYuv(const DecodeFrame& frame, int width, int height);
    void ResetTexNV12(const DecodeFrame& frame);
    void FreeTexYuv();
    void FreeTexNV12();

private:
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
};

#endif
