cmake_minimum_required(VERSION 3.15)

enable_testing()

project(future_demo)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_COMPILER_IS_GUNCXX)
    set(CMAKE_CXX_FLAGS "--coverage")
else()
    set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif()

add_subdirectory(3rd_party/abseil-cpp)
add_subdirectory(3rd_party/googletest)

add_executable(future_demo tests/main.cpp)

target_link_libraries(future_demo gtest pthread absl::flat_hash_map)
