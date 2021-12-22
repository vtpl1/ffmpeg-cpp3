find_package(Catch2 REQUIRED)
find_package(Poco REQUIRED Util)

# These tests can use the Catch2-provided main
add_executable(tests
    test.cpp
)

target_link_libraries(tests
    PRIVATE ffmpegcpp3
    PRIVATE Catch2::Catch2WithMain
    PRIVATE Poco::Util
)

set_property(TARGET tests PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")