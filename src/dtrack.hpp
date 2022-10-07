#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "DTrackSDK.hpp"
#include "nlohmann/json.hpp"

class DTrack {
public:
  DTrack(const std::string &connection);
  ~DTrack();

  nlohmann::json tracking_data() const {
    std::unique_lock<std::mutex> lock(m_data_mutex);
    nlohmann::json tracking_data = m_tracking_data;
    return tracking_data;
  }

private:
  DTrackSDK m_dtrack_sdk;

  std::atomic<bool> m_quit = false;
  std::thread m_receive_thread;
  void ReceiveThread();

  mutable std::mutex m_data_mutex;
  nlohmann::json m_tracking_data;

  nlohmann::json GenerateJSON();
  void LogError();
};
