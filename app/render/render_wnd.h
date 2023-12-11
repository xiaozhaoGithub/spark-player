#ifndef RENDER_WND_H_
#define RENDER_WND_H_

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "util/decode_frame.h"

class RenderWnd
{
public:
    RenderWnd();
    virtual ~RenderWnd();

    virtual void Render(const DecodeFrame&) = 0;
    virtual void update() = 0;
    virtual void setGeometry(const QRect&) = 0;

private:
};

#endif
