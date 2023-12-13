#include "render_wnd_sdl.h"

#include "common/avdef.h"
#include "spdlog/spdlog.h"

std::atomic_flag RenderWndSDL::sdl_inited_ = ATOMIC_FLAG_INIT;

static SDL_PixelFormatEnum get_sdl_fmt(int fmt)
{
    switch (fmt) {
    case PIX_FMT_IYUV:
        return SDL_PIXELFORMAT_IYUV;
    case PIX_FMT_NV12:
        return SDL_PIXELFORMAT_NV12;
    case PIX_FMT_RGB:
        return SDL_PIXELFORMAT_RGB24;
    }

    return SDL_PIXELFORMAT_UNKNOWN;
}

RenderWndSDL::RenderWndSDL(QWidget* parent)
    : QWidget(parent)
    , wnd_(nullptr)
    , renderer_(nullptr)
    , video_tex_(nullptr)
    , frame_w_(0)
    , frame_h_(0)
    , frame_format_(0)
    , frame_pitch_(0)
{
    setStyleSheet("QWidget {background: black;}");

    InitSDL();
}

RenderWndSDL::~RenderWndSDL()
{
    if (video_tex_) {
        SDL_DestroyTexture(video_tex_);
        video_tex_ = nullptr;
    }

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (wnd_) {
        SDL_DestroyWindow(wnd_);
        wnd_ = nullptr;
    }
}

void RenderWndSDL::Render(const DecodeFrame& frame)
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    if (!frame.IsNull()) {
        // Rebuild texture
        if (!video_tex_ || frame_w_ != frame.w || frame_h_ != frame.h || frame_format_ != frame.format) {
            if (video_tex_) {
                SDL_DestroyTexture(video_tex_);
                video_tex_ = nullptr;
            }

            uint32_t fmt = get_sdl_fmt(frame.format);
            video_tex_ =
                SDL_CreateTexture(renderer_, fmt, SDL_TEXTUREACCESS_STREAMING, frame.w, frame.h);
            if (!video_tex_) {
                SPDLOG_ERROR("Faild to create SDL texture desc: {0}.", SDL_GetError());
            }

            void* pixels = nullptr;
            int pitch = 0;
            if (SDL_LockTexture(video_tex_, nullptr, &pixels, &pitch) < 0) {
                return;
            }
            memset(pixels, 0, pitch * frame.h);
            SDL_UnlockTexture(video_tex_);

            frame_w_ = frame.w;
            frame_h_ = frame.h;
            frame_format_ = frame.format;
            frame_pitch_ = pitch;
        }

        SDL_UpdateTexture(video_tex_, nullptr, frame.buf.base, frame_pitch_);

        SDL_RenderCopy(renderer_, video_tex_, nullptr, nullptr);
    }

    SDL_RenderPresent(renderer_);
}

void RenderWndSDL::update()
{
    Render(DecodeFrame());
}

void RenderWndSDL::InitSDL()
{
    // Init only once
    if (!sdl_inited_.test_and_set()) {
        SDL_Init(SDL_INIT_VIDEO);
    }

    wnd_ = SDL_CreateWindowFrom(reinterpret_cast<void*>(winId()));
    if (!wnd_) {
        SPDLOG_ERROR("Faild to create SDL window from this: {0} desc: {1}",
                     static_cast<void*>(this), SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(wnd_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        SPDLOG_WARN("Faild to create SDL accelerated renderer, try a normal renderer.");

        renderer_ = SDL_CreateRenderer(wnd_, -1, 0);
        if (!renderer_) {
            SPDLOG_ERROR("Faild to create SDL renderer desc: {0}.", SDL_GetError());
        }
    }

    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(renderer_, &renderer_info);
    if (renderer_info.num_texture_formats == 0) {
        SPDLOG_ERROR("Faild to create SDL renderer desc: {0}.", SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
}
