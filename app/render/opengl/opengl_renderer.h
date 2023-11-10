#ifndef OPENGL_RENDERER_H_
#define OPENGL_RENDERER_H_

#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <memory>

class OpenGLRenderer : protected QOpenGLFunctions_3_3_Core
{
public:
    OpenGLRenderer(const std::shared_ptr<QOpenGLShaderProgram>& shader_program);
    ~OpenGLRenderer();

    void SetSize(const QVector2D& size);

    void Draw(std::shared_ptr<QOpenGLTexture> texture, const QVector2D& pos,
              const QVector2D& size = QVector2D(10.0f, 10.0f), float rotate = 0.0f,
              const QVector3D& color = QVector3D(1.0f, 1.0f, 1.0f));

    void Draw(GLuint texture, const QVector2D& pos, const QVector2D& size = QVector2D(10.0f, 10.0f),
              float rotate = 0.0f, const QVector3D& color = QVector3D(1.0f, 1.0f, 1.0f));

private:
    void InitRenderData();

private:
    QVector2D size_;
    quint32 vao_;
    std::shared_ptr<QOpenGLShaderProgram> shader_program_;
};
#endif
