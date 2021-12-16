add_executable(rtsp_to_rtmp
    rtsp_to_rtmp.cpp
)

target_link_libraries(rtsp_to_rtmp
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET rtsp_to_rtmp PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")