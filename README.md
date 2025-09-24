# SDN-Inspired Load Balancer (C++ Implementation)

## Project Overview
This project is a C++ re-implementation of an SDN load balancer that originally ran on Mininet + POX (Python).  
Instead of Mininet, it uses plain Linux sockets and multithreading to simulate:
- **Clients** sending requests through a load balancer
- **Servers** (backends) with configurable artificial delays
- A **Load Balancer** proxy that distributes connections according to different scheduling policies

### Tech Stack
- **Language:** C++17
- **Build System:** CMake
- **Concurrency:** `std::thread`, `std::mutex`
- **Networking:** POSIX sockets (TCP)
- **Config Parsing:** [nlohmann/json](https://github.com/nlohmann/json) for reading backend lists

This allows you to experiment with different scheduling policies and measure latency/throughput without requiring Mininet or POX.

---

## Scheduling Algorithms
The load balancer currently supports three policies:

1. **Random**
   - Each incoming client is sent to a random backend.
   - Provides simple load spreading but can be unfair.

2. **Round Robin**
   - Backends are chosen in a cyclic order (1 → 2 → 3 → 1 …).
   - Ensures equal distribution regardless of performance differences.

3. **Response-Time Based (EWMA)**
   - Tracks backend response latency using an **Exponentially Weighted Moving Average (EWMA)**.
   - Always prefers the server with the lowest observed response time.
   - Adapts to load conditions and heterogeneous backend speeds.

---

## Setup Instructions

### Prerequisites
- **Ubuntu 20.04+** (any recent Linux will work)
- **CMake ≥ 3.10**
- **GCC ≥ 9 (C++17 capable)**
- **nlohmann-json** (header-only library for JSON parsing)

Install required packages:
```bash
sudo apt update
sudo apt install -y g++ cmake make git
sudo apt install -y nlohmann-json3-dev   
git clone <this-repo-url> LoadBalancer
cd LoadBalancer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ln -sf build/lb .
ln -sf build/server_sim .
ln -sf build/client_sim .
```

## Instructions to execute
### Start servers
```bash
./tools/run_servers.sh
```
This launches three servers:
- Port 9001 with 0.2s artificial delay
- Port 9002 with 0.6s artificial delay
- Port 9003 with 1.0s artificial delay

### Start Load Balancer
```bash
# Response-time (EWMA)
./build/lb config/servers.json --policy=response

# Round-robin
./build/lb config/servers.json --policy=round_robin

# Random
./build/lb config/servers.json --policy=random
```
By default it listens on 127.0.0.1:8080 and proxies to the backends.

### Start Clients
```bash
./tools/run_clients.sh 5
```
Each client will:
- Connect to the load balancer
- Send 20 requests
- Sleep ~5s between requests
- Record stats and append total runtime to times.txt

### Kill Everything
- Stop LB: Ctrl+C in its terminal
- Stop servers: ./tools/kill_all.sh
