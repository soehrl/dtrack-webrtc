#include "webcave_server.hpp"

#include <chrono>
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "websocketpp/connection.hpp"

WebCaveServer::WebCaveServer(const Options& options)
  : m_options(options), m_dtrack(options.dtrack_connection) {
}

WebCaveServer::~WebCaveServer() {
  Stop();
}

int WebCaveServer::Run() {
  m_websocket_server.init_asio();

  m_websocket_server.set_open_handler([this](const auto& connection) {
      std::unique_lock<std::mutex> lock(m_connections_mutex);
      m_connections.insert(std::make_pair(connection, Client{}));
  });
  m_websocket_server.set_close_handler([this](const auto& connection) {
      std::unique_lock<std::mutex> lock(m_connections_mutex);
      m_connections.erase(connection);
  });
  m_websocket_server.set_message_handler([this](const auto& connection_handle, const auto& message) {
    assert(m_connections.find(connection_handle) != m_connections.end());
    try {
      const auto parsed_message = nlohmann::json::parse(message->get_payload());
      if (parsed_message["type"] == "frameReady") {
        bool ready = true;
        // std::unique_lock<std::mutex> lock(m_connections_mutex);
        {
          m_connections[connection_handle].frame = parsed_message["frame"];
          for (const auto& [_, connection_data] : m_connections) {
            if (connection_data.frame != m_current_frame - 1) {
              ready = false;
              break;
            }
          }
        }
        if (ready) {
          Broadcast({
            { "type", "displayFrame" },
            { "frame", m_current_frame - 1 },
          });
        }
      } else {
        spdlog::warn("Unknown message from {}: {}", "TODO", parsed_message.dump(1));
      }
    } catch (const std::exception& error) {
      spdlog::error("{}", error.what());
    }
  });

  spdlog::info("Starting server on port {}", m_options.port);
  m_websocket_server.listen(m_options.port);
  m_websocket_server.start_accept();

  m_update_thread = std::thread(&WebCaveServer::UpdateThread, this);
  m_websocket_server.run();

  return EXIT_SUCCESS;
}

void WebCaveServer::Stop() {
  spdlog::info("Stopping");

  if (!m_quit) {
    m_quit = true;
    m_update_thread.join();

    {
      std::unique_lock<std::mutex> lock(m_connections_mutex);
      for (const auto& [connection_handle, x] : m_connections) {
        m_websocket_server.close(connection_handle, 1001, "Server shutdown");
      }
      m_connections.clear();
    }
    m_websocket_server.stop();
  }
}

void WebCaveServer::UpdateThread() {
  spdlog::info("Running updates at {}Hz", m_options.update_rate);

  using Clock = std::chrono::steady_clock;
  const auto delta_time = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(1.0 / m_options.update_rate));

  double time = 0.0;

  auto time_last_frame = Clock::now();
  while (!m_quit.load(std::memory_order_relaxed)) {
    const auto now = Clock::now();
    if (now - time_last_frame >= delta_time) {
      fmt::print("\rFrame: {}, Time: {:0.3}", m_current_frame, time);

      // Do not set time_last_frame to now but instead add
      // delta_time to it to avoid slow drift over time.
      time_last_frame += delta_time;

      if (std::unique_lock<std::mutex> connections_lock(m_connections_mutex);
          !m_connections.empty()) {
        connections_lock.unlock();
        Broadcast({
          { "type", "startFrame" },
          { "frame", m_current_frame },
          { "time", time },
          { "deltaTime", 1.0 / m_options.update_rate },
          { "trackingData", m_dtrack.tracking_data() },
        });
        ++m_current_frame;
        time = m_current_frame / m_options.update_rate;
      }
    }
  }
}

void WebCaveServer::Broadcast(const nlohmann::json& message) {
  const std::string data = message.dump();
  std::unique_lock<std::mutex> lock(m_connections_mutex);
  for (const auto& [connection_handle, x] : m_connections) {
    try {
      m_websocket_server.send(connection_handle, data.data(), data.size(),
                              websocketpp::frame::opcode::TEXT);
    } catch (const websocketpp::exception& error) {
      spdlog::error("{}", error.what());
    }
  }
}

// <<<<<<< Updated upstream
// #include "websocket_server.hpp"
// #include "spdlog/spdlog.h"
// #include "websocketpp/server.hpp"
// #include "websocketpp/config/asio_no_tls.hpp"
// #include <mutex>
// #include <set>

// static std::optional<std::thread> thread;
// using server_t = websocketpp::server<websocketpp::config::asio>;
// static websocketpp::server<websocketpp::config::asio> server;
// std::mutex m;
// std::deque<nlohmann::json> messages;

// std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections;

// void websocket_runner();

// void on_open(websocketpp::connection_hdl hdl) {
// }

// void on_close(websocketpp::connection_hdl hdl) {
//   std::unique_lock<std::mutex> m;
//   connections.erase(hdl);
// }

// void start_server(uint16_t port) {
//   if (thread.has_value()) {
//     return;
//   }
// }

// void quit_server() {
//   server.stop();
//   thread->join();
// }

// void broadcast_message(nlohmann::json message) {
//   const std::string message_data = message.dump();
//   std::unique_lock<std::mutex> m;
//   for (const auto& connection : connections) {
//     try {
//       server.send(connection, message_data.data(), message_data.size(), websocketpp::frame::opcode::TEXT);
//     } catch (const websocketpp::exception& error) {
//       spdlog::error("Websocket error: {}", error.what());
//     }
//   }
// }

// void websocket_runner() {

//   server.run();
// }
// =======
// #include "websocket_server.hpp"
// #include "spdlog/spdlog.h"
// #include <mutex>
// #include <vector>


// void websocket_runner();

// void on_open(websocketpp::connection_hdl hdl) {
//   std::unique_lock<std::mutex> m;
//   connections.push_back(hdl);
// }

// void start_server(uint16_t port) {
//   if (thread.has_value()) {
//     return;
//   }
//   server.init_asio();

//   server.set_open_handler(on_open);
//   server.listen(port);
//   server.start_accept();

//   thread.emplace(websocket_runner);
// }

// void quit_server() {
//   server.stop();
//   thread->join();
// }

// void broadcast_message(nlohmann::json message) {
//   const std::string message_data = message.dump();
//   std::unique_lock<std::mutex> m;
//   for (const auto& connection : connections) {
//     server.send(connection, message_data.data(), message_data.size(), websocketpp::frame::opcode::TEXT);
//   }
// }

// void websocket_runner() {

//   server.run();
// }
//


// >>>>>>> Stashed changes
