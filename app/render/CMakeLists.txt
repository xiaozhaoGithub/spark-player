add_subdirectory(qpainter)
add_subdirectory(opengl)
add_subdirectory(sdl2)

set(Sources
	${Sources}
	render/render_factory.h
	render/render_wnd.h
	render/render_wnd.cc
	PARENT_SCOPE
)
