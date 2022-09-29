cmake_minimum_required(
  VERSION 3.14
  FATAL_ERROR
)

project(
  dtrack-webrtc
  VERSION 0.0.0
  LANGUAGES CXX
)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:gabime/spdlog@1.10.0")
CPMAddPackage("gh:soehrl/dtrack-cmake-wrapper#main")

add_executable(
  dtrack-webrtc

  src/main.cpp
)

target_link_libraries(
  dtrack-webrtc
  PRIVATE
    fmt::fmt
    spdlog::spdlog
    dtrack::dtrack
)

set_property(
  TARGET dtrack-webrtc
  PROPERTY CXX_STANDARD 17
)
