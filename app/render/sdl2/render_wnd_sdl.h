#ifndef RENDER_WND_SDL_H_
#define RENDER_WND_SDL_H_

#include "SDL.h"
#include "render/render_wnd.h"
#include "util/decode_frame.h"

class RenderWndSDL : public RenderWnd, public QWidget
{
public:
    explicit RenderWndSDL(QWidget* parent = nullptr);
    ~RenderWndSDL();

protected:
    void Render(const DecodeFrame& frame) override;
    void setGeometry(const QRect& rect) override { QWidget::setGeometry(rect); }
    void update() override { QWidget::update(); }

private:
    static std::atomic_flag sdl_inited_;
    SDL_Window* wnd_;
    SDL_Renderer* renderer_;
    SDL_Texture* video_tex_;

    int frame_w_;
    int frame_h_;
    int frame_format_;
    int frame_pitch_;
};

#endif
