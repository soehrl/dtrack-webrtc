#include "websocket_server.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/server.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include <mutex>
#include <set>

static std::optional<std::thread> thread;
using server_t = websocketpp::server<websocketpp::config::asio>;
static websocketpp::server<websocketpp::config::asio> server;
std::mutex m;
std::deque<nlohmann::json> messages;

std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections;

void websocket_runner();

void on_open(websocketpp::connection_hdl hdl) {
  std::unique_lock<std::mutex> m;
  connections.insert(hdl);
}

void on_close(websocketpp::connection_hdl hdl) {
  std::unique_lock<std::mutex> m;
  connections.erase(hdl);
}

void start_server(uint16_t port) {
  if (thread.has_value()) {
    return;
  }
  server.init_asio();

  server.set_open_handler(on_open);
  server.set_close_handler(on_close);
  server.listen(port);
  server.start_accept();

  thread.emplace(websocket_runner);
}

void quit_server() {
  server.stop();
  thread->join();
}

void broadcast_message(nlohmann::json message) {
  const std::string message_data = message.dump();
  std::unique_lock<std::mutex> m;
  for (const auto& connection : connections) {
    try {
      server.send(connection, message_data.data(), message_data.size(), websocketpp::frame::opcode::TEXT);
    } catch (const websocketpp::exception& error) {
      spdlog::error("Websocket error: {}", error.what());
    }
  }
}

void websocket_runner() {

  server.run();
}
