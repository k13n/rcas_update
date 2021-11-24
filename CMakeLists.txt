cmake_minimum_required (VERSION 3.0)
project (app)

# enable compiler warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -g -fno-omit-frame-pointer")

# set the C++ version
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# # show make output
# set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(src)
add_subdirectory(tests)

add_executable(benchmark_cache_misses1 ${CMAKE_CURRENT_SOURCE_DIR}/apps/benchmark_cache_misses1.cpp)
target_link_libraries(benchmark_cache_misses1 cas)

add_executable(benchmark_cache_misses2 ${CMAKE_CURRENT_SOURCE_DIR}/apps/benchmark_cache_misses2.cpp)
target_link_libraries(benchmark_cache_misses2 cas)

add_executable(benchmark_merge_query ${CMAKE_CURRENT_SOURCE_DIR}/apps/benchmark_merge_query.cpp)
target_link_libraries(benchmark_merge_query cas)

add_executable(benchmark_insertion ${CMAKE_CURRENT_SOURCE_DIR}/apps/benchmark_insertion.cpp)
target_link_libraries(benchmark_insertion cas)

add_executable(benchmark_deletion ${CMAKE_CURRENT_SOURCE_DIR}/apps/benchmark_deletion.cpp)
target_link_libraries(benchmark_deletion cas)

add_executable(app ${CMAKE_CURRENT_SOURCE_DIR}/apps/app.cpp)
target_link_libraries(app cas)
