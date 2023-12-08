#ifndef FAST_LAYOUT_H_
#define FAST_LAYOUT_H_

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

class CHBoxLayout : public QHBoxLayout
{
public:
    CHBoxLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

class CVBoxLayout : public QVBoxLayout
{
public:
    CVBoxLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

class CGridLayout : public QGridLayout
{
public:
    CGridLayout(QWidget* parent = nullptr, int margin = 0, int spacing = 0);
};

#endif
