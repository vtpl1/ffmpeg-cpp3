add_executable(remuxing
    remuxing.cpp
)

target_link_libraries(remuxing
    PUBLIC ffmpeg_cpp3
)

set_property(TARGET remuxing PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")