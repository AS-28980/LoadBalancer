#include "backend.hpp"
#include <arpa/inet.h>
#include <poll.h>

int Backend::connect_with_timeout(int timeout_ms) const {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;

  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(addr.port);
  if (::inet_pton(AF_INET, addr.ip.c_str(), &sa.sin_addr) != 1) {
    ::close(fd);
    errno = EINVAL;
    return -1;
  }

  // non-blocking connect + poll
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  int rc = ::connect(fd, (sockaddr*)&sa, sizeof(sa));
  if (rc == 0) {
    // connected immediately
    fcntl(fd, F_SETFL, flags);
    return fd;
  }
  if (errno != EINPROGRESS) {
    ::close(fd);
    return -1;
  }

  pollfd pfd{fd, POLLOUT, 0};
  rc = ::poll(&pfd, 1, timeout_ms);
  if (rc == 1 && (pfd.revents & POLLOUT)) {
    int err = 0; socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
      ::close(fd);
      errno = err ? err : errno;
      return -1;
    }
    fcntl(fd, F_SETFL, flags);
    return fd;
  }
  ::close(fd);
  errno = ETIMEDOUT;
  return -1;
}
