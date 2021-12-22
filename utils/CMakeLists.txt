cmake_minimum_required(VERSION 3.17)
find_package(spdlog REQUIRED)

add_library(utils STATIC
  src/logging.cpp
  inc/logging.h
)

target_include_directories(utils
  PUBLIC inc
)

target_link_libraries(utils
  PUBLIC spdlog::spdlog
)

set_property(TARGET utils PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")