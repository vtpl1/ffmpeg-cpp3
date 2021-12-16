add_executable(demo
    demo.cpp
    GeneratedAudioSource.cpp
    GeneratedVideoSource.cpp
    GeneratedAudioSource.h
    GeneratedVideoSource.h
)

target_link_libraries(demo
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET demo PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")