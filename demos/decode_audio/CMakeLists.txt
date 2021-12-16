add_executable(decode_audio
    decode_audio.cpp
)

target_link_libraries(decode_audio
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET decode_audio PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")