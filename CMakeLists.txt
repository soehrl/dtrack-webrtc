cmake_minimum_required(
  VERSION 3.14
  FATAL_ERROR
)

project(
  dtrack-web
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

add_executable(
  dtrack-web

  src/main.cpp
  src/websocket_server.cpp
)

target_link_libraries(
  dtrack-web
  PRIVATE
    fmt::fmt
    spdlog::spdlog
    dtrack::dtrack
    nlohmann_json::nlohmann_json
)

target_include_directories(
  dtrack-web
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/websocketpp-src
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/asio-src/asio/include
)

target_compile_definitions(
  dtrack-web
  PRIVATE
    -DASIO_STANDALONE=1
)

set_property(
  TARGET dtrack-web
  PROPERTY CXX_STANDARD 17
)
