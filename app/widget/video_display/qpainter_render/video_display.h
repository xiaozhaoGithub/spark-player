#ifndef VIDEO_DISPLAY_H_
#define VIDEO_DISPLAY_H_

#include <QPaintEvent>
#include <QPainter>
#include <QWidget>
#include <memory>
#include <mutex>

#include "render/video_renderer.h"

class VideoPlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPlayerWidget(QWidget* parent = nullptr);
    ~VideoPlayerWidget();

    void Open(const char* name);
    void Pause();
    void Stop();
    void set_pixmap(const QPixmap& pixmap);

signals:
    void PlayState(bool playing);

private slots:
    void UpdateImage(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::mutex mutex_;
    QPixmap pixmap_;

    VideoRenderer* video_renderer_;
};

#endif
