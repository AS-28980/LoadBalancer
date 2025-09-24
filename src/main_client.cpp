#include "client_sim.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  // Usage: client_sim [ip] [port]
  ClientConfig cfg;
  if (argc >= 2) cfg.target.ip = argv[1];
  if (argc >= 3) cfg.target.port = static_cast<uint16_t>(std::stoi(argv[2]));
  std::cout << "Client target " << cfg.target.ip << ":" << cfg.target.port << "\n";
  std::this_thread::sleep_for(std::chrono::seconds(30)); // mimic original 30s delay
  SimClient cli(cfg);
  cli.run();
  return 0;
}
