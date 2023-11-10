#include "video_surface_gl.h"

#include "spdlog/spdlog.h"
#include <QOpenGLShaderProgram>

VideoSurfaceGL::VideoSurfaceGL(QWidget* parent)
    : QOpenGLWidget(parent)
{
    video_renderer_ = new VideoCodecManager(this);
    connect(video_renderer_, &VideoCodecManager::SendFrame, this, &VideoSurfaceGL::ProcessFrame);
    connect(video_renderer_, &VideoCodecManager::PlayState, this, &VideoSurfaceGL::PlayState);

    InitMenu();
}

VideoSurfaceGL::~VideoSurfaceGL() {}

void VideoSurfaceGL::Open(const char* name)
{
    video_renderer_->Open(name);
}

void VideoSurfaceGL::Pause()
{
    video_renderer_->Pause();
}

void VideoSurfaceGL::Stop()
{
    video_renderer_->Stop();
}

void VideoSurfaceGL::ProcessFrame(AVFrame* frame)
{
    if (!frame || frame->width == 0 || frame->height == 0)
        return;

    if (frame->width != frame_size_.width() || frame->height != frame_size_.height()) {
        if (y_tex_ && u_tex_ && v_tex_) {
            y_tex_->destroy();
            u_tex_->destroy();
            v_tex_->destroy();

            y_tex_.reset();
            u_tex_.reset();
            v_tex_.reset();
        }
    }

    if (!y_tex_) {
        y_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        u_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        v_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);

        y_tex_->setSize(frame->width, frame->height);
        u_tex_->setSize(frame->width / 2, frame->height / 2);
        v_tex_->setSize(frame->width / 2, frame->height / 2);

        y_tex_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
        u_tex_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
        v_tex_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);

        y_tex_->setFormat(QOpenGLTexture::R8_UNorm);
        u_tex_->setFormat(QOpenGLTexture::R8_UNorm);
        v_tex_->setFormat(QOpenGLTexture::R8_UNorm);

        y_tex_->allocateStorage();
        u_tex_->allocateStorage();
        v_tex_->allocateStorage();

        frame_size_.setWidth(frame->width);
        frame_size_.setHeight(frame->height);
    }

    pix_transfer_opts_.setImageHeight(frame->height);

    pix_transfer_opts_.setRowLength(frame->linesize[0]);
    y_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[0], &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame->linesize[1]);
    u_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[1], &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame->linesize[2]);
    v_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[2], &pix_transfer_opts_);

    av_frame_unref(frame);

    update();
}

void VideoSurfaceGL::FullScreenClicked()
{
    if (isFullScreen())
        return;

    setWindowFlags(Qt::Window);

    // Direct full screen will only be full screen on the main screen and placed in the Qt queue.
    QMetaObject::invokeMethod(this, "showFullScreen", Qt::QueuedConnection);
}

void VideoSurfaceGL::ExitFullScreenClicked()
{
    if (!isFullScreen())
        return;

    setWindowFlags(Qt::Widget);
    showNormal();
}

void VideoSurfaceGL::initializeGL()
{
    auto shader_program = std::make_shared<QOpenGLShaderProgram>();
    shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/res/shaders/yuv2rgb.vert");
    shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/res/shaders/yuv2rgb.frag");
    if (!shader_program->link()) {
        SPDLOG_ERROR(shader_program->log().toStdString());
    }

    if (!renderer_) {
        renderer_ = std::make_shared<OpenGLRenderer>(shader_program);
    }

    initializeOpenGLFunctions();
}

void VideoSurfaceGL::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);

    renderer_->SetSize(QVector2D(w, h));
}

void VideoSurfaceGL::paintGL()
{
    renderer_->Draw(y_tex_, u_tex_, v_tex_, QVector2D(width(), height()));
}

void VideoSurfaceGL::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);

    menu_->popup(mapToGlobal(event->pos()));
}

void VideoSurfaceGL::InitMenu()
{
    menu_ = new QMenu(this);
    menu_->addAction(tr("full screen"), this, &VideoSurfaceGL::FullScreenClicked);
    menu_->addAction(tr("exit full screen"), this, &VideoSurfaceGL::ExitFullScreenClicked);
}
