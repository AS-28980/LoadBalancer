#pragma once
#include "lb_strategy.hpp"
#include <atomic>
#include <nlohmann/json.hpp>
#include <fstream>

struct LBConfig {
  SockAddr listen { "0.0.0.0", 8080 };
  std::string policy = "response";
  double alpha = 0.10;        // EWMA weight for new sample
  double decay_other = 0.70;  // decay on non-selected
  int connect_timeout_ms = 1500;
  std::vector<Backend> backends;
};

class LoadBalancer {
public:
  explicit LoadBalancer(const LBConfig& cfg);
  void serve();  // blocking accept loop

private:
  LBConfig cfg_;
  std::unique_ptr<ILBPolicy> policy_;
  std::mutex mtx_; // protects backends metrics
  int listen_fd_ = -1;
  std::atomic<bool> stop_{false};

  size_t choose_backend(); // uses policy_
  void update_metrics_on_pick(size_t idx);
  void update_metrics_on_response(size_t idx, int64_t init_us, int64_t first_byte_us);
  void decay_others(size_t chosen_idx);

  static int create_listen_socket(const SockAddr& sa);
  static void proxy_conn(int client_fd, const Backend& backend, LoadBalancer* self, size_t backend_idx, int connect_timeout_ms);
  static void pump(int from_fd, int to_fd, std::atomic<bool>& open_from, std::atomic<bool>& open_to, std::optional<int64_t>& first_byte_ts);
};

LBConfig load_config(const std::string& path);
