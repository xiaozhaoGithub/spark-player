#ifndef VIDEO_DISPLAY_H_
#define VIDEO_DISPLAY_H_

#include <QContextMenuEvent>
#include <QLabel>
#include <QMenu>
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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void InitMenu();

private slots:
    void UpdateImage(const QImage& image);
    void FullScreenClicked();
    void ExitFullScreenClicked();

private:
    std::mutex mutex_;
    QPixmap pixmap_;
    VideoRenderer* video_renderer_;

    int divide_num_;

    QMenu* menu_;
};

#endif
