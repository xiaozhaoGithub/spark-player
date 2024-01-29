set(Sources
    main.cc
)

add_subdirectory(codec)
add_subdirectory(render)
add_subdirectory(window)
add_subdirectory(widget)
add_subdirectory(dialog)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(util)
add_subdirectory(media_play)
add_subdirectory(audio)

QT5_ADD_RESOURCES(RCC_FILES spark-player.qrc)

if (WIN32)
	# Preserve folder structure in visual studio
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})
elseif(UNIX)
endif()

add_executable(${PROJECT_NAME} 
	${Sources}
	${RCC_FILES}
)

target_include_directories(${PROJECT_NAME} 
PRIVATE
	${FFMPEG_DEMO_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE 
	${FFMPEG_DEMO_LIBRARIES}
)

target_link_directories(${PROJECT_NAME} 
PRIVATE
	${FFMPEG_DEMO_LIB_INCLUDE_DIRS}
)
