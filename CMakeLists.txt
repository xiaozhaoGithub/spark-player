cmake_minimum_required(VERSION 3.5)

project(ffmpeg-player LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED 
COMPONENTS 
	Widgets 
)
LIST(APPEND FFMPEG_DEMO_LIBRARIES
	Qt${QT_VERSION_MAJOR}::Widgets
)

LIST(APPEND FFMPEG_DEMO_LIBRARIES
	avcodec
	avdevice
	avfilter
	avformat
	avutil
	postproc
	swresample
	swscale
)
LIST(APPEND FFMPEG_DEMO_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/ffmpeg/include)
LIST(APPEND FFMPEG_DEMO_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)

LIST(APPEND FFMPEG_DEMO_LIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib)

add_subdirectory(app)

