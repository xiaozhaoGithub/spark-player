#include "video_surface_gl.h"

#include "spdlog/spdlog.h"
#include <QOpenGLShaderProgram>

VideoSurfaceGL::VideoSurfaceGL(QWidget* parent)
    : QOpenGLWidget(parent)
{
    video_renderer_ = new VideoCodecManager(this);
    connect(video_renderer_, &VideoCodecManager::UpdateImage, this, &VideoSurfaceGL::UpdateImage);
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

void VideoSurfaceGL::UpdateImage(const QImage& image)
{
    if (image.isNull())
        return;

    video_tex_->destroy();
    video_tex_->setData(image);

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
    shader_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/res/shaders/sprite.vert");
    shader_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/res/shaders/sprite.frag");
    if (!shader_program->link()) {
        SPDLOG_ERROR(shader_program->log().toStdString());
    }

    if (!renderer_) {
        renderer_ = std::make_shared<OpenGLRenderer>(shader_program);
    }

    if (!video_tex_) {
        video_tex_ = std::make_shared<QOpenGLTexture>(QOpenGLTexture::Target2D);
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
    if (!video_tex_)
        return;

    renderer_->Draw(video_tex_, QVector2D(0.0f, 0.0f), QVector2D(width(), height()));
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
