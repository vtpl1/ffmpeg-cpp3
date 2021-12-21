# *****************************************************
#    Copyright 2021 Videonetics Technology Pvt Ltd
# *****************************************************

cmake_minimum_required(VERSION 3.16)
project(ffmpegcpp3)
message(STATUS "VCPKG_INSTALLED_DIR " ${VCPKG_INSTALLED_DIR})
list(APPEND CMAKE_MODULE_PATH ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/ffmpeg)
message(STATUS "CMAKE_MODULE_PATH" ${CMAKE_MODULE_PATH})

find_package(FFMPEG REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;)

add_library(ffmpegcpp3 STATIC
    src/CoreObject.cpp
    src/CoreContext.cpp
    src/FFmpegException.cpp
    src/OpenCodec.cpp
    src/CodecDeducer.cpp
    src/FrameContainer.cpp
    src/Sources/InputSource.cpp
    src/Sources/Demuxer.cpp
    src/Demuxing/InputStream.cpp
    src/Demuxing/VideoInputStream.cpp
    src/Demuxing/AudioInputStream.cpp
    src/FrameSinks/FrameSinkStream.cpp

    src/SimpleInputOutput/InputToMessageQueue.cpp
    src/SimpleInputOutput/OutputFromMessageQueue.cpp


    inc/ffmpeg.h
    inc/CoreObject.h
    inc/CoreContext.h
    inc/FFmpegException.h
    inc/OpenCodec.h
    inc/CodecDeducer.h
    inc/FrameContainer.h
    inc/Sources/InputSource.h
    inc/Sources/Demuxer.h
    inc/Demuxing/StreamData.h
    inc/Demuxing/InputStream.h
    inc/Demuxing/VideoInputStream.h
    inc/Demuxing/AudioInputStream.h
    inc/Info/VideoStreamInfo.h
    inc/Info/AudioStreamInfo.h
    inc/Info/ContainerInfo.h
    inc/FrameSinks/FrameSink.h
    inc/FrameSinks/FrameSinkStream.h

    inc/SimpleInputOutput/InputToMessageQueue.h
    inc/SimpleInputOutput/OutputFromMessageQueue.h


)

target_compile_features(ffmpegcpp3 PRIVATE cxx_std_17)

target_include_directories(ffmpegcpp3
    PUBLIC inc
    PUBLIC inc/Sources
    PUBLIC inc/Demuxing
    PUBLIC inc/Info
    PUBLIC inc/FrameSinks
    PUBLIC inc/SimpleInputOutput
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

add_subdirectory(entrypoint)