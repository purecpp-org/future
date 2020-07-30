cmake_minimum_required(VERSION 3.15)

enable_testing()

project(future_demo)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_COMPILER_IS_GUNCXX)
    set(CMAKE_CXX_FLAGS "--coverage")
else()
    set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif()

add_definitions(-DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        -DBOOST_THREAD_PROVIDES_EXECUTORS
        -DBOOST_THREAD_USES_MOVE
        )

include_directories(
        /usr/local/include
)

link_directories(
        /usr/local/lib
)

#add_subdirectory(3rd_party/abseil-cpp)
#add_subdirectory(3rd_party/googletest)

add_executable(future_demo tests/future_test.cc)

target_link_libraries(future_demo boost_thread gtest pthread -labsl_bad_variant_access )
