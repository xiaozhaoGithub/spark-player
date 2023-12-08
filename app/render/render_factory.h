#ifndef RENDER_FACTORY_H_
#define RENDER_FACTORY_H_

#include "render/opengl/render_wnd_gl.h"
#include "render/render_wnd.h"

enum RenderType
{
    kOpenGL,
    kSDL2,
};

class RenderFactory
{
public:
    static RenderWnd* Create(RenderType type, QWidget* parent)
    {
        switch (type) {
        case kOpenGL:
            return new RenderWndGL(parent);
        case kSDL2:
            return nullptr;
        default:
            return nullptr;
        }
    }
};

#endif
