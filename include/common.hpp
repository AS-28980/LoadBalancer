#pragma once
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

inline void die(const std::string& msg) {
  std::cerr << "[FATAL] " << msg << " (errno=" << errno << ": " << std::strerror(errno) << ")\n";
  std::exit(1);
}

inline void set_reuse(int fd) {
  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    die("setsockopt(SO_REUSEADDR) failed");
}

inline void set_nonblock(int fd, bool nb=true) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) die("fcntl(F_GETFL) failed");
  if (fcntl(fd, F_SETFL, nb ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0)
    die("fcntl(F_SETFL) failed");
}

using Clock = std::chrono::steady_clock;
inline int64_t now_us() {
  return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count();
}

inline int64_t ms_since(int64_t start_us) {
  return (now_us() - start_us) / 1000;
}

struct SockAddr {
  std::string ip;
  uint16_t port{};
};
