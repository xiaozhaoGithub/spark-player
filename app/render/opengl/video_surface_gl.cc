#include "video_surface_gl.h"

#include "spdlog/spdlog.h"
#include <QOpenGLShaderProgram>

VideoSurfaceGL::VideoSurfaceGL(QWidget* parent)
    : QOpenGLWidget(parent)
{
    video_thread_ = new VideoWorkerThread(this);
    connect(video_thread_, &VideoWorkerThread::SendFrame, this, &VideoSurfaceGL::ProcessFrame);
    connect(video_thread_, &VideoWorkerThread::PlayState, this, &VideoSurfaceGL::PlayState);
    connect(video_thread_, &VideoWorkerThread::RecordState, this, &VideoSurfaceGL::RecordState);

    InitMenu();
}

VideoSurfaceGL::~VideoSurfaceGL() {}

void VideoSurfaceGL::Open()
{
    video_thread_->Open();
}

void VideoSurfaceGL::Pause()
{
    video_thread_->Pause();
}

void VideoSurfaceGL::Stop()
{
    video_thread_->Stop();
}

void VideoSurfaceGL::StartRecord()
{
    video_thread_->StartRecord();
}

void VideoSurfaceGL::StopRecord()
{
    video_thread_->StopRecord();
}

void VideoSurfaceGL::set_media(const MediaInfo& media)
{
    video_thread_->set_media(media);
}

void VideoSurfaceGL::ProcessFrame(AVFrame* frame)
{
    if (!frame || frame->width == 0 || frame->height == 0)
        return;

    int tex_width = frame->width;
    int tex_height = frame->height;

    format_ = frame->format;
    switch (format_) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P: {
        tex_width = tex_width / 2;
        tex_height = tex_height / 2;
        ResetTexYuv(frame, format_, tex_width, tex_height);
    }
    case AV_PIX_FMT_YUVJ422P: {
        tex_width = tex_width / 2;
        tex_height = tex_height;
        ResetTexYuv(frame, format_, tex_width, tex_height);
        break;
    }
    case AV_PIX_FMT_NV12: {
        ResetTexNV12(frame);
        break;
    }
    default:
        break;
    }
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
    switch (format_) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
    case AV_PIX_FMT_YUVJ422P: {
        renderer_->Draw(y_tex_, u_tex_, v_tex_, format_, QVector2D(width(), height()));
        break;
    }
    case AV_PIX_FMT_NV12: {
        renderer_->Draw(y_tex_, uv_tex_, format_, QVector2D(width(), height()));
        break;
    }
    default:
        break;
    }
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

void VideoSurfaceGL::ReallocTex(QOpenGLTexturePtr tex, int type, int width, int height, int depth)
{
    tex->setSize(width, height);
    tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    tex->setFormat(static_cast<QOpenGLTexture::TextureFormat>(type));
    tex->allocateStorage();
}

void VideoSurfaceGL::ResetTexYuv(AVFrame* frame, int type, int width, int height)
{
    if (frame->width != frame_size_.width() || frame->height != frame_size_.height()) {
        FreeTexYuv();
    }

    if (!y_tex_) {
        y_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(y_tex_, QOpenGLTexture::R8_UNorm, frame->width, frame->height);
    }
    if (!u_tex_) {
        u_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(u_tex_, QOpenGLTexture::R8_UNorm, width, height);
    }
    if (!v_tex_) {
        v_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(v_tex_, QOpenGLTexture::R8_UNorm, width, height);
    }

    frame_size_.setWidth(frame->width);
    frame_size_.setHeight(frame->height);

    pix_transfer_opts_.setImageHeight(frame->height);

    pix_transfer_opts_.setRowLength(frame->linesize[0]);
    y_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[0], &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame->linesize[1]);
    u_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[1], &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame->linesize[2]);
    v_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[2], &pix_transfer_opts_);
}

void VideoSurfaceGL::ResetTexNV12(AVFrame* frame)
{
    if (frame->width != frame_size_.width() || frame->height != frame_size_.height()) {
        FreeTexNV12();
    }

    if (!y_tex_) {
        y_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(y_tex_, QOpenGLTexture::R8_UNorm, frame->width, frame->height);
    }
    if (!uv_tex_) {
        uv_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(uv_tex_, QOpenGLTexture::RG8_UNorm, frame->width / 2, frame->height / 2);
    }

    frame_size_.setWidth(frame->width);
    frame_size_.setHeight(frame->height);

    pix_transfer_opts_.setImageHeight(frame->height);
    pix_transfer_opts_.setRowLength(frame->linesize[0]);
    y_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame->data[0], &pix_transfer_opts_);

    // pix_transfer_opts_.setImageHeight(frame->height / 2);
    pix_transfer_opts_.setRowLength(frame->linesize[1] / 2); // uv interleaved

    uv_tex_->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, frame->data[1], &pix_transfer_opts_);
}

void VideoSurfaceGL::FreeTexYuv()
{
    y_tex_.reset();
    u_tex_.reset();
    v_tex_.reset();
}

void VideoSurfaceGL::FreeTexNV12()
{
    y_tex_.reset();
    uv_tex_.reset();
}
