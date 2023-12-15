#ifndef FAST_LAYOUT_H_
#define FAST_LAYOUT_H_

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

class CHBoxLayout : public QHBoxLayout
{
public:
    explicit CHBoxLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

class CVBoxLayout : public QVBoxLayout
{
public:
    explicit CVBoxLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

class CGridLayout : public QGridLayout
{
public:
    explicit CGridLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

#endif
