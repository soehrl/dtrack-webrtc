#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

void start_server(uint16_t port);
void quit_server();
void broadcast_message(nlohmann::json message);
