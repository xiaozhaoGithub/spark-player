#include "render_wnd_sdl.h"

#include "SDL.h"
#include "spdlog/spdlog.h"

std::atomic_flag RenderWndSDL::sdl_inited_ = ATOMIC_FLAG_INIT;

RenderWndSDL::RenderWndSDL(QWidget* /*parent*/)
{
    // Init only once
    if (!sdl_inited_.test_and_set()) {
        SDL_Init(SDL_INIT_VIDEO);
    }

    SDL_Window* wnd = SDL_CreateWindowFrom((void*)winId());
    if (!wnd) {
        SPDLOG_ERROR("Faild to create SDL window from this: {0} desc: {1}",
                     static_cast<void*>(this), SDL_GetError());
    }
}

RenderWndSDL::~RenderWndSDL() {}

void RenderWndSDL::Render(const DecodeFrame& frame) {}
