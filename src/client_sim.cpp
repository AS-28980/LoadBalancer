#include "client_sim.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <random>

void SimClient::single_request() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) die("client socket");

  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(cfg_.target.port);
  if (::inet_pton(AF_INET, cfg_.target.ip.c_str(), &sa.sin_addr) != 1) die("client inet_pton");

  const int64_t t0 = now_us();
  if (::connect(fd, (sockaddr*)&sa, sizeof(sa)) < 0) die("client connect");

  std::string req = "Client request: Hello, server!";
  if (::send(fd, req.data(), req.size(), 0) < 0) die("client send");

  std::vector<char> buf(cfg_.recv_chunk);
  ssize_t n = ::recv(fd, buf.data(), buf.size(), 0); // read first chunk
  if (n < 0) die("client recv");
  last_bytes_ = static_cast<size_t>(n);

  // Try to parse "BACKEND <port>\n" from the start of the stream
  std::string first(buf.data(), buf.data() + n);
  std::string backend = "unknown";
  if (first.rfind("BACKEND ", 0) == 0) { // starts with "BACKEND "
    auto nl = first.find('\n');
    if (nl != std::string::npos) backend = first.substr(8, nl - 8);
  }

  last_latency_s_ = (now_us() - t0) / 1e6;
  total_time_s_ += last_latency_s_;
  ::close(fd);

  if (backend != "unknown") {
    std::cout << "→ Served by backend port: " << backend << "\n";
  }
}

void SimClient::print_stats() const {
  if (last_latency_s_ <= 0) {
    std::cout << "\nNo data received yet.\n";
    return;
  }
  double throughput_kb_s = (last_bytes_ / 1024.0) / last_latency_s_;
  std::cout << "Received " << last_bytes_ << " bytes in " << last_latency_s_
            << " s | Throughput: " << throughput_kb_s << " KB/s\n";
}

void SimClient::run() {
  auto uni = std::uniform_real_distribution<double>(0.0, cfg_.jitter_s);
  const int64_t T0 = now_us();
  for (int i = 0; i < cfg_.iterations; ++i) {
    std::cout << "Connecting to " << cfg_.target.ip << ":" << cfg_.target.port << " ...\n";
    single_request();
    print_stats();
    double wait_s = cfg_.base_wait_s + uni(rng_);
    std::cout << "Waiting " << wait_s << " s before next request...\n\n";
    std::this_thread::sleep_for(std::chrono::duration<double>(wait_s));
  }
  double tot_time_s = (now_us() - T0) / 1e6;
  std::ofstream out("times.txt", std::ios::app);
  out << tot_time_s << "\n";
  std::cout << "Total client run time appended to times.txt: " << tot_time_s << " s\n";
}
