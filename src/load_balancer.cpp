#include "load_balancer.hpp"
#include "common.hpp"
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <atomic>
#include <fstream>

using json = nlohmann::json;

static std::mutex g_csv_mtx;
static std::unique_ptr<std::ofstream> g_csv;
static void ensure_csv_open() {
  static std::once_flag once;
  std::call_once(once, []{
    const char* p = std::getenv("LB_CSV_PATH");
    if (p && *p) {
      g_csv = std::make_unique<std::ofstream>(p, std::ios::app);
      if (*g_csv) {
        *g_csv << "t_us,policy,backend_ip,backend_port,latency_us,ewma_before_us,ewma_after_us\n";
      }
    }
  });
}


LBConfig load_config(const std::string& path) {
  std::ifstream in(path);
  if (!in) die("cannot open " + path);
  json j; in >> j;

  LBConfig cfg;
  cfg.listen.ip   = j.value("listen_ip", std::string("0.0.0.0"));
  cfg.listen.port = j.value("listen_port", 8080);
  cfg.policy      = j.value("policy", std::string("response"));
  cfg.alpha       = j.value("alpha", 0.10);
  cfg.decay_other = j.value("decay_other", 0.70);
  cfg.connect_timeout_ms = j.value("connect_timeout_ms", 1500);

  for (auto& b : j.at("backends")) {
    Backend be;
    be.addr.ip = b.at("ip").get<std::string>();
    be.addr.port = b.at("port").get<uint16_t>();
    cfg.backends.push_back(std::move(be));
  }
  if (cfg.backends.empty()) die("no backends configured");
  return cfg;
}

static std::unique_ptr<ILBPolicy> make_policy(const std::string& p) {
  if (p == "random")      return std::make_unique<RandomPolicy>();
  if (p == "round_robin") return std::make_unique<RoundRobinPolicy>();
  if (p == "response")    return std::make_unique<ResponseTimePolicy>();
  die("unknown policy: " + p);
  return {};
}

LoadBalancer::LoadBalancer(const LBConfig& cfg) : cfg_(cfg), policy_(make_policy(cfg.policy)) {
  listen_fd_ = create_listen_socket(cfg_.listen);
}

int LoadBalancer::create_listen_socket(const SockAddr& sa) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) die("socket failed");

  set_reuse(fd);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(sa.port);
  if (::inet_pton(AF_INET, sa.ip.c_str(), &addr.sin_addr) != 1) die("inet_pton failed");

  if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) die("bind failed");
  if (::listen(fd, 1024) < 0) die("listen failed");
  return fd;
}

size_t LoadBalancer::choose_backend() {
  std::lock_guard<std::mutex> lk(mtx_);
  return policy_->choose(cfg_.backends);
}

void LoadBalancer::update_metrics_on_pick(size_t idx) {
  std::lock_guard<std::mutex> lk(mtx_);
  cfg_.backends[idx].last_init_us = now_us();
  decay_others(idx);
}

void LoadBalancer::update_metrics_on_response(size_t idx, int64_t init_us, int64_t first_byte_us) {
  if (first_byte_us <= 0) return;
  const double sample = static_cast<double>(first_byte_us - init_us);
  std::lock_guard<std::mutex> lk(mtx_);
  auto& b = cfg_.backends[idx];
  double ewma_before = b.ewma_us;
  b.last_done_us = first_byte_us;
  b.ewma_us = cfg_.alpha * sample + (1.0 - cfg_.alpha) * b.ewma_us;
  b.success_count++;

  ensure_csv_open();
  if (g_csv && *g_csv) {
    std::lock_guard<std::mutex> lk2(g_csv_mtx);
    *g_csv << now_us() << "," << cfg_.policy << ","
           << b.addr.ip << "," << b.addr.port << ","
           << (first_byte_us - init_us) << ","
           << ewma_before << "," << b.ewma_us << "\n";
  }
}


void LoadBalancer::decay_others(size_t chosen_idx) {
  for (size_t i = 0; i < cfg_.backends.size(); ++i) {
    if (i == chosen_idx) continue;
    cfg_.backends[i].ewma_us *= cfg_.decay_other;
  }
}

void LoadBalancer::pump(int from_fd, int to_fd, std::atomic<bool>& open_from, std::atomic<bool>& open_to,
                        std::optional<int64_t>& first_byte_ts) {
  constexpr size_t BUF_SZ = 64 * 1024;
  std::vector<char> buf(BUF_SZ);
  while (open_from && open_to) {
    ssize_t n = ::recv(from_fd, buf.data(), buf.size(), 0);
    if (n == 0) break;        // EOF
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (!first_byte_ts.has_value()) first_byte_ts = now_us();
    ssize_t off = 0;
    while (off < n) {
        #ifdef MSG_NOSIGNAL
        ssize_t m = ::send(to_fd, buf.data() + off, n - off, MSG_NOSIGNAL);
        #else
        ssize_t m = ::send(to_fd, buf.data() + off, n - off, 0);
        #endif
      if (m < 0) {
        if (errno == EINTR) continue;
        open_to = false;
        return;
      }
      off += m;
    }
  }
  open_from = false;
  ::shutdown(to_fd, SHUT_WR); // half-close
}

void LoadBalancer::proxy_conn(int client_fd, const Backend& backend, LoadBalancer* self, size_t backend_idx, int connect_timeout_ms) {
  int64_t init_us = now_us();

  int be_fd = backend.connect_with_timeout(connect_timeout_ms);
  if (be_fd < 0) {
    ::close(client_fd);
    std::lock_guard<std::mutex> lk(self->mtx_);
    self->cfg_.backends[backend_idx].failure_count++;
    return;
  }

  std::atomic<bool> open_cli{true}, open_be{true};
  std::optional<int64_t> first_byte_from_be;
  std::optional<int64_t> first_byte_from_cli;

  // two pumps: client->backend and backend->client
  std::thread t_up([&]{ pump(client_fd, be_fd, open_cli, open_be, first_byte_from_cli); });
  std::thread t_dn([&]{ pump(be_fd, client_fd, open_be, open_cli, first_byte_from_be); });

  t_up.join();
  t_dn.join();

  ::close(client_fd);
  ::close(be_fd);

  if (first_byte_from_be.has_value()) {
    self->update_metrics_on_response(backend_idx, init_us, first_byte_from_be.value());
  }
}

void LoadBalancer::serve() {
  std::cout << "[LB] Listening on " << cfg_.listen.ip << ":" << cfg_.listen.port
            << " with policy=" << cfg_.policy << "\n";
  for (size_t i = 0; i < cfg_.backends.size(); ++i) {
    std::cout << "  backend[" << i << "] = " << cfg_.backends[i].addr.ip << ":" << cfg_.backends[i].addr.port
              << " ewma_us=" << cfg_.backends[i].ewma_us << "\n";
  }

  while (!stop_) {
    sockaddr_in cli{}; socklen_t len = sizeof(cli);
    int cfd = ::accept(listen_fd_, (sockaddr*)&cli, &len);
    if (cfd < 0) {
      if (errno == EINTR) continue;
      die("accept failed");
    }

    size_t idx = choose_backend();
    update_metrics_on_pick(idx);
    Backend chosen;
    { std::lock_guard<std::mutex> lk(mtx_); chosen = cfg_.backends[idx]; }

    std::thread(&LoadBalancer::proxy_conn, cfd, chosen, this, idx, cfg_.connect_timeout_ms).detach();
  }
}
