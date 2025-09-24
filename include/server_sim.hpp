#pragma once
#include "common.hpp"

struct ServerConfig {
  SockAddr listen { "0.0.0.0", 9001 };
  int backlog = 200;
  double work_delay_s = 0.0;
  size_t first_chunk_bytes = 40960; // 40 KB
};

class SimServer {
public:
  explicit SimServer(ServerConfig cfg): cfg_(std::move(cfg)) {}
  void run(); // blocking

private:
  ServerConfig cfg_;
};
