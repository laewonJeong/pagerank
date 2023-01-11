cmake_minimum_required(VERSION 3.16)

project(pagerank)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/lib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/network)

add_executable (pagerank ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(pagerank PRIVATE lib)
target_link_libraries(pagerank PRIVATE threadpool)
target_link_libraries(pagerank PRIVATE network)
target_compile_features(pagerank PRIVATE cxx_std_17)
