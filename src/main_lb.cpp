#include "load_balancer.hpp"
#include <iostream>
#include <regex>
#include <csignal>
int main(int argc, char** argv) {
  std::signal(SIGPIPE, SIG_IGN);
  std::string cfg_path = "config/servers.json";
  std::string override_policy;     // "", "random", "round_robin", "response"
  std::string csv_path;            // e.g. "logs/lb_random.csv"

  if (argc >= 2) cfg_path = argv[1];
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind("--policy=", 0) == 0) override_policy = a.substr(9);
    else if (a.rfind("--csv=", 0) == 0) csv_path = a.substr(5);
  }

  LBConfig cfg = load_config(cfg_path);
  if (!override_policy.empty()) cfg.policy = override_policy;
  if (!csv_path.empty()) setenv("LB_CSV_PATH", csv_path.c_str(), 1); // pass via env to LB
  LoadBalancer lb(cfg);
  lb.serve();
  return 0;
}
