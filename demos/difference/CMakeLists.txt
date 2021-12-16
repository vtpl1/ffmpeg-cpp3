add_executable(difference
difference.cpp
)

target_link_libraries(difference
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET difference PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")