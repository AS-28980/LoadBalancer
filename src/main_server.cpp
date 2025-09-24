#include "server_sim.hpp"
#include <iostream>
#include <csignal>

int main(int argc, char** argv) {
  std::signal(SIGPIPE, SIG_IGN);
  // Usage: server_sim [port] [delay_seconds]
  ServerConfig cfg;
  if (argc >= 2) cfg.listen.port = static_cast<uint16_t>(std::stoi(argv[1]));
  if (argc >= 3) cfg.work_delay_s = std::stod(argv[2]);

  SimServer s(cfg);
  s.run();
  return 0;
}
