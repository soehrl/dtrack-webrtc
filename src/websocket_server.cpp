#include "websocket_server.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/server.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include <mutex>
#include <vector>

static std::optional<std::thread> thread;
using server_t = websocketpp::server<websocketpp::config::asio>;
static websocketpp::server<websocketpp::config::asio> server;
std::mutex m;
std::deque<nlohmann::json> messages;
std::vector<websocketpp::connection_hdl> connections;

void websocket_runner();

void on_open(websocketpp::connection_hdl hdl) {
  std::unique_lock<std::mutex> m;
  connections.push_back(hdl);
}

void start_server(uint16_t port) {
  if (thread.has_value()) {
    return;
  }
  server.init_asio();

  server.set_open_handler(on_open);
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
    server.send(connection, message_data.data(), message_data.size(), websocketpp::frame::opcode::TEXT);
  }
}

void websocket_runner() {

  server.run();
}
