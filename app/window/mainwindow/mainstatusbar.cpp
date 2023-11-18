#include "mainstatusbar.h"

MainStatusBar::MainStatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    showMessage("Development phase");
}

MainStatusBar::~MainStatusBar() {}
