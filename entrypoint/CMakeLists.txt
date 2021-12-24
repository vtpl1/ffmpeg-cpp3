cmake_minimum_required(VERSION 3.17)
find_package(spdlog REQUIRED)
find_package( Poco REQUIRED Util Net )
# find_package( Poco REQUIRED Util Net )

add_executable(media_converter
  EntryPoint.cpp
)

target_link_libraries(media_converter
  PRIVATE ffmpegcpp3
  PRIVATE Poco::Util
  PRIVATE Poco::Net
  PRIVATE utils
)

set_property(TARGET media_converter PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")