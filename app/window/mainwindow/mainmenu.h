#ifndef MAINMENU_H_
#define MAINMENU_H_

#include <QMenuBar>

class MainMenu : public QMenuBar
{
    Q_OBJECT
public:
    explicit MainMenu(QWidget* parent = nullptr);
    ~MainMenu();

private slots:
    void DecodeAudio();
};

#endif
