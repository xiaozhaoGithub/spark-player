add_subdirectory(ffmpeg)

set(Sources
	${Sources}
	media_play/video_player_factory.h
	media_play/video_player.h
	media_play/video_player.cc
	PARENT_SCOPE
)
