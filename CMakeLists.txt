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

message(STATUS "AVCODEC_INCLUDE_DIR ${AVCODEC_INCLUDE_DIR}")
message(STATUS "SWRESAMPLE_LIBRARY ${SWRESAMPLE_LIBRARY}")
message(STATUS "AVUTIL_LIBRARY ${AVUTIL_LIBRARY}")

file(GLOB_RECURSE INC sources/*.h)
file(GLOB_RECURSE SRC sources/*.cpp)

add_library(${PROJECT_NAME} ${SRC})


target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/sources
    PUBLIC ${AVCODEC_INCLUDE_DIR}
    PUBLIC ${AVFORMAT_INCLUDE_DIR}
    PUBLIC ${AVUTIL_INCLUDE_DIR}
    PUBLIC ${AVFILTER_INCLUDE_DIR}
)


target_link_libraries(${PROJECT_NAME}
    PUBLIC ${AVDEVICE_LIBRARY}
    PUBLIC ${AVFILTER_LIBRARY}
    PUBLIC ${SWSCALE_LIBRARY}
    PUBLIC ${AVFORMAT_LIBRARY}
    PUBLIC ${AVCODEC_LIBRARY}
    PUBLIC ${SWRESAMPLE_LIBRARY}
    PUBLIC ${AVUTIL_LIBRARY}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")