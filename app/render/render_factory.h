#ifndef RENDER_FACTORY_H_
#define RENDER_FACTORY_H_

#include "render/opengl/render_wnd_gl.h"
#include "render/render_wnd.h"
#include "render/sdl2/render_wnd_sdl.h"

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
            return new RenderWndSDL(parent);
        default:
            return nullptr;
        }
    }
};

#endif
