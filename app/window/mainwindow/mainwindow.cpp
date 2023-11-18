#include "mainwindow.h"

#include <QMenuBar>

#include "mainmenu.h"
#include "mainstatusbar.h"
#include "widget/video_display/video_display_widget.h"

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    auto main_menu = new MainMenu(this);
    setMenuBar(main_menu);

    auto status_bar = new MainStatusBar(this);
    setStatusBar(status_bar);

    auto w = new VideoDisplayWidget(this);
    setCentralWidget(w);
}

MainWindow::~MainWindow() {}
