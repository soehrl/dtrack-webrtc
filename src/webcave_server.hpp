#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <thread>

#include "dtrack.hpp"
#include "options.hpp"
#include "websocketpp/server.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include "nlohmann/json.hpp"

struct Client {
  std::optional<std::uint64_t> frame;
};

class WebCaveServer {
 public:
  WebCaveServer(const Options& options);
  ~WebCaveServer();

  int Run();
  void Stop();

 private:
  Options m_options;

  std::atomic<bool> m_quit = false;
  void UpdateThread();
  std::thread m_update_thread;

  using ServerType = websocketpp::server<websocketpp::config::asio>;
  ServerType m_websocket_server;

  std::mutex m_connections_mutex;
  std::map<websocketpp::connection_hdl, Client, std::owner_less<websocketpp::connection_hdl>> m_connections;

  DTrack m_dtrack;

  std::uint64_t m_current_frame = 0;

  void Broadcast(const nlohmann::json& message);
};
