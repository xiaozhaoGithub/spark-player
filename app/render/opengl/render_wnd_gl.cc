#include "render_wnd_gl.h"

#include <QOpenGLShaderProgram>

#include "media_play/video_player_factory.h"
#include "spdlog/spdlog.h"

RenderWndGL::RenderWndGL(QWidget* parent)
    : QOpenGLWidget(parent)
{
    InitUi();
}

RenderWndGL::~RenderWndGL() {}

void RenderWndGL::Open()
{
    video_player_->Open();
}

void RenderWndGL::Pause()
{
    video_player_->Pause();
}

void RenderWndGL::Stop()
{
    video_player_->Stop();
}

void RenderWndGL::StartRecord()
{
    // video_player_->StartRecord();
}

void RenderWndGL::StopRecord()
{
    // video_player_->StopRecord();
}

void RenderWndGL::set_media(const MediaInfo& media)
{
    video_player_->set_media(media);
}

void RenderWndGL::OnRender()
{
    DecodeFrame frame;
    video_player_->pop_frame(&frame);
}

void RenderWndGL::ProcessFrame(const DecodeFrame& frame)
{
    if (frame.w == 0 || frame.h == 0)
        return;

    int tex_width = frame.w;
    int tex_height = frame.h;

    format_ = frame.format;
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

    update();
}

void RenderWndGL::FullScreenClicked()
{
    if (isFullScreen())
        return;

    setWindowFlags(Qt::Window);

    // Direct full screen will only be full screen on the main screen and placed in the Qt queue.
    QMetaObject::invokeMethod(this, "showFullScreen", Qt::QueuedConnection);
}

void RenderWndGL::ExitFullScreenClicked()
{
    if (!isFullScreen())
        return;

    setWindowFlags(Qt::Widget);
    showNormal();
}

void RenderWndGL::initializeGL()
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

void RenderWndGL::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);

    renderer_->SetSize(QVector2D(w, h));
}

void RenderWndGL::paintGL()
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

void RenderWndGL::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);

    menu_->popup(mapToGlobal(event->pos()));
}

void RenderWndGL::InitUi()
{
    video_player_ = VideoPlayerFactory::Create(kFile);
    /*  connect(video_player_, &VideoWorkerThread::PlayState, this, &RenderWndGL::PlayState);
      connect(video_player_, &VideoWorkerThread::RecordState, this, &RenderWndGL::RecordState);*/

    InitMenu();

    render_timer_ = new QTimer(this);
    render_timer_->setTimerType(Qt::PreciseTimer);
    connect(render_timer_, &QTimer::timeout, this, &RenderWndGL::OnRender);
}

void RenderWndGL::InitMenu()
{
    menu_ = new QMenu(this);
    menu_->addAction(tr("Full Screen"), this, &RenderWndGL::FullScreenClicked);
    menu_->addAction(tr("Exit Full Screen"), this, &RenderWndGL::ExitFullScreenClicked);
}

void RenderWndGL::ReallocTex(QOpenGLTexturePtr tex, int type, int width, int height, int depth)
{
    tex->setSize(width, height);
    tex->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    tex->setFormat(static_cast<QOpenGLTexture::TextureFormat>(type));
    tex->allocateStorage();
}

void RenderWndGL::ResetTexYuv(const DecodeFrame& frame, int type, int width, int height)
{
    if (frame.w != frame_size_.width() || frame.h != frame_size_.height()) {
        FreeTexYuv();
    }

    if (!y_tex_) {
        y_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(y_tex_, QOpenGLTexture::R8_UNorm, frame.w, frame.h);
    }
    if (!u_tex_) {
        u_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(u_tex_, QOpenGLTexture::R8_UNorm, width, height);
    }
    if (!v_tex_) {
        v_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(v_tex_, QOpenGLTexture::R8_UNorm, width, height);
    }

    frame_size_.setWidth(frame.w);
    frame_size_.setHeight(frame.h);

    pix_transfer_opts_.setImageHeight(frame.h);

    int y_size = frame.w * frame.h;

    pix_transfer_opts_.setRowLength(frame.w);
    y_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.buf.base, &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame.w / 2);
    u_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.buf.base + y_size,
                    &pix_transfer_opts_);

    pix_transfer_opts_.setRowLength(frame.w / 2);
    v_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8,
                    frame.buf.base + y_size + y_size / 4, &pix_transfer_opts_);
}

void RenderWndGL::ResetTexNV12(const DecodeFrame& frame)
{
    if (frame.w != frame_size_.width() || frame.h != frame_size_.height()) {
        FreeTexNV12();
    }

    if (!y_tex_) {
        y_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(y_tex_, QOpenGLTexture::R8_UNorm, frame.w, frame.h);
    }
    if (!uv_tex_) {
        uv_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
        ReallocTex(uv_tex_, QOpenGLTexture::RG8_UNorm, frame.w / 2, frame.h / 2);
    }

    frame_size_.setWidth(frame.w);
    frame_size_.setHeight(frame.h);

    pix_transfer_opts_.setImageHeight(frame.h);
    pix_transfer_opts_.setRowLength(frame.w);
    y_tex_->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.buf.base, &pix_transfer_opts_);

    // pix_transfer_opts_.setImageHeight(frame.h/ 2);
    // pix_transfer_opts_.setRowLength(frame.linesize[1] / 2); // uv interleaved

    // uv_tex_->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, frame.data[1], &pix_transfer_opts_);
}

void RenderWndGL::FreeTexYuv()
{
    y_tex_.reset();
    u_tex_.reset();
    v_tex_.reset();
}

void RenderWndGL::FreeTexNV12()
{
    y_tex_.reset();
    uv_tex_.reset();
}
