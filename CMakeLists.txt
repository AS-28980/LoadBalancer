cmake_minimum_required(VERSION 3.16)
project(sdnless_lb CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Release)

add_compile_options(-Wall -Wextra -Wpedantic -O2 -pthread)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(lb
  src/load_balancer.cpp
  src/backend.cpp
  src/main_lb.cpp)

add_executable(server_sim
  src/backend.cpp
  src/server_sim.cpp   
  src/main_server.cpp)

add_executable(client_sim
  src/client_sim.cpp
  src/main_client.cpp)

target_link_libraries(lb pthread)
target_link_libraries(server_sim pthread)
target_link_libraries(client_sim pthread)
