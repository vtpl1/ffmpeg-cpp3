cmake_minimum_required(VERSION 3.17)
find_package(spdlog REQUIRED)
find_package( Poco REQUIRED Util Net )
# find_package( Poco REQUIRED Util Net )

add_executable(ffmpeg_cpp3_tester
  EntryPoint.cpp
)

target_link_libraries(ffmpeg_cpp3_tester
  PRIVATE ffmpegcpp3
  PRIVATE Poco::Util
  PRIVATE Poco::Net
  PRIVATE utils
)

set_property(TARGET ffmpeg_cpp3_tester PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")