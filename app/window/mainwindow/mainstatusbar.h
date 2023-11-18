#ifndef MAINSTATUSBAR_H_
#define MAINSTATUSBAR_H_

#include <QStatusBar>

class MainStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit MainStatusBar(QWidget* parent = nullptr);
    ~MainStatusBar();
};

#endif
