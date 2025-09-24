#pragma once
#include "backend.hpp"
#include <random>

class ILBPolicy {
public:
  virtual ~ILBPolicy() = default;
  virtual size_t choose(const std::vector<Backend>& backends) = 0;
};

class RandomPolicy : public ILBPolicy {
  std::mt19937_64 rng{std::random_device{}()};
public:
  size_t choose(const std::vector<Backend>& b) override {
    std::uniform_int_distribution<size_t> d(0, b.size()-1);
    return d(rng);
  }
};

class RoundRobinPolicy : public ILBPolicy {
  size_t last_ = static_cast<size_t>(-1);
public:
  size_t choose(const std::vector<Backend>& b) override {
    last_ = (last_ + 1) % b.size();
    return last_;
  }
};

class ResponseTimePolicy : public ILBPolicy {
public:
  size_t choose(const std::vector<Backend>& b) override {
    size_t best = 0;
    double best_val = b[0].ewma_us;
    for (size_t i = 1; i < b.size(); ++i) {
      if (b[i].ewma_us < best_val) { best = i; best_val = b[i].ewma_us; }
    }
    return best;
  }
};
