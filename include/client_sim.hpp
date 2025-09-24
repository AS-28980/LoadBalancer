#pragma once
#include "common.hpp"
#include <random>

struct ClientConfig {
  SockAddr target { "127.0.0.1", 8080 };
  int iterations = 500;
  double base_wait_s = 0.0;
  double jitter_s = 0.0;  // uniform [0,1)
  size_t recv_chunk = 40960;
};

class SimClient {
public:
  explicit SimClient(ClientConfig cfg): cfg_(std::move(cfg)) {}
  void run();

private:
  ClientConfig cfg_;
  std::mt19937 rng_{std::random_device{}()};
  double total_time_s_ = 0.0;
  size_t last_bytes_ = 0;
  double last_latency_s_ = 0.0;

  void single_request();
  void print_stats() const;
};
