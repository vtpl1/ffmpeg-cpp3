add_executable(encode_audio
    encode_audio.cpp
)

target_link_libraries(encode_audio
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET encode_audio PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")