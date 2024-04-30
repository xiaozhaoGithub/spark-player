add_subdirectory(sdl2)
add_subdirectory(portaudio)

set(Sources
	${Sources}
	audio/audiomanager.h
	audio/audiomanager.cc
	audio/audio_play_ctrl.h
	audio/audio_play_ctrl.cc
	PARENT_SCOPE
)
