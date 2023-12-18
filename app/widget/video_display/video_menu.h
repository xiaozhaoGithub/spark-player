#ifndef VIDEO_MENU_H_
#define VIDEO_MENU_H_

#include <QMenu>

class VideoMenu : public QObject
{
    Q_OBJECT
public:
    explicit VideoMenu(QWidget* parent);
    ~VideoMenu();

    void ShowMenu(const QPoint& pos);

    void SetFullscreenState(bool fullscreen);
    void SetRecordState(bool start);

signals:
    void FullScreen();
    void ExitFullScreen();
    void StopRecording();
    void StartRecording();

private:
    QMenu* menu_;

    QAction* fullscreen_;
    QAction* exit_fullscreen_;
    QAction* start_recording_;
    QAction* stop_recording_;
};

#endif
