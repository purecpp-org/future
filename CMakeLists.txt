cmake_minimum_required(VERSION 3.12)
enable_testing()
project(future_demo)

# link libraries
set(LINK_LIBRARIES
    gtest
)

######## compiler options #######
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
set(CMAKE_CXX_STANDARD 17)
message("using C++17 standard...")
else()
set(CMAKE_CXX_STANDARD 11)
message("using C++11 standard...")
list(APPEND LINK_LIBRARIES pthread)
endif()

if(CMAKE_COMPILE_IS_GUNCXX)
    set(CMAKE_CXX_FLAGS "--coverage")
else()
    set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif()
################################

# boost
find_package(Boost 1.71.0 REQUIRED
    COMPONENTS thread)
message("Boost Version:${Boost_VERSION} found.")
message(${Boost_INCLUDE_DIRS})
message(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
add_definitions(
        -DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        -DBOOST_THREAD_PROVIDES_EXECUTORS
        -DBOOST_THREAD_USES_MOVE
        )

# 3rd party
OPTION(COPMILE_WITH_3RD_PARTY "Compile with 3rd party libraries." OFF)
if(COPMILE_WITH_3RD_PARTY)
    add_subdirectory(3rd_party/abseil-cpp)
    add_subdirectory(3rd_party/googletest)
    list(APPEND LINK_LIBRARIES absl::flat_hash_map)
else()
    include_directories(
        /usr/local/include
    )

    link_directories(
        /usr/local/lib
    )
    list(APPEND LINK_LIBRARIES -labsl_bad_variant_access)
endif()

add_executable(${PROJECT_NAME} tests/future_test.cc)
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES} ${Boost_LIBRARIES})
