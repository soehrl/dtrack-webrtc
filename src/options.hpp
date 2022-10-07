#pragma once

#include <cstdint>
#include <string>

struct Options {
  uint16_t port = 5000;
  double update_rate = 60;
  std::string dtrack_connection;
};
