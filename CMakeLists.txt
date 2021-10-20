cmake_minimum_required(VERSION 3.16)
project(edgelist2metis)

set(CMAKE_CXX_STANDARD 17)

add_executable(edgelist2metis main.cpp definitions.h progress.h read_edgelist.h write_metis.h)
target_link_libraries(edgelist2metis tbb)
