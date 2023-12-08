add_subdirectory(ffmpeg)

set(Sources
	${Sources}
	media_play/stream_event_type.h
	media_play/video_player.h
	media_play/video_player.cc
	media_play/video_player_factory.h
	PARENT_SCOPE
)
