cmake_minimum_required(VERSION 3.15)
project(future_demo)

set(CMAKE_CXX_STANDARD 11)

include_directories(
        /usr/local/include
)

link_directories(
        /usr/local/lib
)

add_executable(future_demo tests/main.cpp)

target_link_libraries(future_demo gtest pthread -labsl_bad_variant_access)
