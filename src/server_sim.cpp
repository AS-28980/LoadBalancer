#include "server_sim.hpp"
#include <arpa/inet.h>

void SimServer::run() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) die("server socket");
  set_reuse(fd);

  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(cfg_.listen.port);
  if (::inet_pton(AF_INET, cfg_.listen.ip.c_str(), &sa.sin_addr) != 1) die("server inet_pton");

  if (::bind(fd, (sockaddr*)&sa, sizeof(sa)) < 0) die("server bind");
  if (::listen(fd, cfg_.backlog) < 0) die("server listen");

  std::cout << "Server started at " << cfg_.listen.ip << ":" << cfg_.listen.port
            << " with delay " << cfg_.work_delay_s << " s. Waiting for connections...\n";

  const std::string big(cfg_.first_chunk_bytes, 'D');
  const std::string resp = "Server response: Work completed! Server is available now.";
  const std::string id = "BACKEND " + std::to_string(cfg_.listen.port) + "\n";

  while (true) {
    sockaddr_in ca{}; socklen_t len = sizeof(ca);
    int cfd = ::accept(fd, (sockaddr*)&ca, &len);
    if (cfd < 0) {
      if (errno == EINTR) continue;
      die("server accept");
    }

    std::thread([=]() {
      std::this_thread::sleep_for(std::chrono::duration<double>(cfg_.work_delay_s));

      ::send(cfd, id.data(), id.size(), 0);
      ssize_t s1 = ::send(cfd, big.data(), big.size(), 0);
      (void)s1;
      ssize_t s2 = ::send(cfd, resp.data(), resp.size(), 0);
      (void)s2;
      ::close(cfd);
    }).detach();
  }
}
