#ifndef RENDER_WND_SDL_H_
#define RENDER_WND_SDL_H_


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
};

#endif
