add_executable(decode_video
decode_video.cpp
)

target_link_libraries(decode_video
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET decode_video PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")