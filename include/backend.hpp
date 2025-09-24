#pragma once
#include "common.hpp"

struct Backend {
  SockAddr addr;
  // response-time EWMA in microseconds; initialize to big number so it's not auto-picked as min=0
  double ewma_us = 3e6;      // 3 seconds default
  int64_t last_init_us = 0;  // timestamp when we forwarded client request to this backend
  int64_t last_done_us = 0;  // timestamp of first byte seen from backend after forwarding
  int success_count = 0;
  int failure_count = 0;

  // connection helper
  int connect_with_timeout(int timeout_ms) const;
};
