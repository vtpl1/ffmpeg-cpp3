# *****************************************************
#    Copyright 2021 Videonetics Technology Pvt Ltd
# *****************************************************

cmake_minimum_required(VERSION 3.16)
project(ffmpegcpp3)

find_package(ffmpeg REQUIRED)
find_package(Threads REQUIRED)

add_library(ffmpegcpp3 STATIC
    src/FFmpegException.cpp
    src/OpenCodec.cpp
    src/Sources/InputSource.cpp
    src/Sources/Demuxer.cpp

    inc/ffmpeg.h
    inc/FFmpegException.h
    inc/OpenCodec.h
    inc/Sources/InputSource.h
    inc/Sources/Demuxer.h
)

target_compile_features(ffmpegcpp3 PRIVATE cxx_std_17)

target_include_directories(ffmpegcpp3
    PUBLIC inc
    PUBLIC inc/Sources
    PUBLIC ${FFMPEG_INCLUDE_DIRS}
)

target_compile_definitions(ffmpegcpp3
    PUBLIC NOMINMAX
)

target_link_libraries(ffmpegcpp3
    PUBLIC ${FFMPEG_LIBRARIES}
    PUBLIC Threads::Threads
)

set_property(TARGET ffmpegcpp3 PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(tests)