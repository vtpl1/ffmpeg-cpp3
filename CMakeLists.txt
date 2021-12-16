cmake_minimum_required(VERSION 3.17)

project(ffmpeg_cpp3)

message(STATUS "VCPKG_INSTALLED_DIR " ${VCPKG_INSTALLED_DIR})

set(FFMPEG_DIR ${VCPKG_INSTALLED_DIR})

if (DEFINED FFMPEG_INCLUDE_DIR)
	set(FFMPEG_INCLUDE_DIR "${FFMPEG_INCLUDE_DIR}" CACHE PATH "Path to FFmpeg includes")
else()
	set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/include")
endif()

if (DEFINED FFMPEG_LIB_DIR)
	set(FFMPEG_LIB_DIR "${FFMPEG_LIB_DIR}" CACHE PATH "Path to FFmpeg libraries")
else()
	set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/lib")
endif()

message(STATUS "Searching for FFmpeg libs in ${FFMPEG_LIB_DIR}")
message(STATUS "Searching for FFmpeg headers in ${FFMPEG_INCLUDE_DIR}")

find_path( AVCODEC_INCLUDE_DIR libavcodec/avcodec.h
    PATHS ${FFMPEG_INCLUDE_DIR})
find_path( AVFORMAT_INCLUDE_DIR libavformat/avformat.h
    PATHS ${FFMPEG_INCLUDE_DIR})
find_path( AVUTIL_INCLUDE_DIR libavutil/avutil.h
    PATHS ${FFMPEG_INCLUDE_DIR})
find_path( AVFILTER_INCLUDE_DIR libavfilter/avfilter.h
    PATHS ${FFMPEG_INCLUDE_DIR})

find_library( SWRESAMPLE_LIBRARY swresample
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVFORMAT_LIBRARY avformat
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVCODEC_LIBRARY avcodec
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVUTIL_LIBRARY avutil
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVFILTER_LIBRARY avfilter
    PATHS ${FFMPEG_LIB_DIR})
find_library( AVDEVICE_LIBRARY avdevice
    PATHS ${FFMPEG_LIB_DIR})
find_library( SWSCALE_LIBRARY swscale
    PATHS ${FFMPEG_LIB_DIR})
find_library( POSTPROC_LIBRARY postproc
    PATHS ${FFMPEG_LIB_DIR})

find_package(OpenSSL)
find_package(ffmpeg REQUIRED)

file(GLOB_RECURSE INC sources/*.h)
file(GLOB_RECURSE SRC sources/*.cpp)

add_library(ffmpeg_cpp3 ${SRC})


target_include_directories(ffmpeg_cpp3
    PUBLIC ${PROJECT_SOURCE_DIR}/sources
    PUBLIC ${FFMPEG_INCLUDE_DIRS}
)


target_link_libraries(ffmpeg_cpp3
    PUBLIC ${FFMPEG_LIBRARIES}
)

set_property(TARGET ffmpeg_cpp3 PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(demos/decode_audio)
add_subdirectory(demos/decode_video)
add_subdirectory(demos/demo)
add_subdirectory(demos/difference)
add_subdirectory(demos/remuxing)
add_subdirectory(demos/rtsp_to_rtmp)
