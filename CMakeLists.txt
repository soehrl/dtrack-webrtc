cmake_minimum_required(
  VERSION 3.14
  FATAL_ERROR
)

project(
  webcave-server
  VERSION 0.0.0
  LANGUAGES CXX
)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:nlohmann/json@3.11.2")
CPMAddPackage("gh:gabime/spdlog@1.10.0")
CPMAddPackage("gh:soehrl/dtrack-cmake-wrapper#main")
CPMAddPackage("gh:zaphoyd/websocketpp#0.8.2")
CPMAddPackage("gh:chriskohlhoff/asio#asio-1-24-0")
CPMAddPackage("gh:adishavit/argh@1.3.2")

add_executable(
  webcave-server

  src/main.cpp
  src/webcave_server.cpp
  src/dtrack.cpp
)

target_link_libraries(
  webcave-server
  PRIVATE
    fmt::fmt
    spdlog::spdlog
    dtrack::dtrack
    nlohmann_json::nlohmann_json
    argh
)

target_include_directories(
  webcave-server
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/websocketpp-src
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/asio-src/asio/include
)

target_compile_definitions(
  webcave-server
  PRIVATE
    -DASIO_STANDALONE=1
)

set_property(
  TARGET webcave-server
  PROPERTY CXX_STANDARD 17
)
