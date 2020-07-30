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

add_executable(${PROJECT_NAME} tests/main.cpp)
target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})
