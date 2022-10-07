#include <chrono>
#include <iostream>
#include <optional>

#include "argh.h"

#include "options.hpp"
#include "webcave_server.hpp"

const std::string_view help_string = R"(webcave-server
)";

std::optional<WebCaveServer> server;

void handle_signint(int) {
  if (server) {
    server->Stop();
  }
}

int main(int argc, char* argv[]) {
  argh::parser cmdl({
    "-r", "--update-rate",
    "-d", "--dtrack",
    "-p", "--port",
  });
  cmdl.parse(argc, argv);

  Options options;
  cmdl("update-rate") >> options.update_rate;
  cmdl("port") >> options.port;
  cmdl("dtrack") >> options.dtrack_connection;

  struct sigaction sigint_handler;
  sigint_handler.sa_handler = handle_signint;
  sigemptyset(&sigint_handler.sa_mask);
  sigint_handler.sa_flags = 0;

  sigaction(SIGINT, &sigint_handler, nullptr);

  server.emplace(options);

  return server->Run();
}
