cmake_minimum_required(VERSION 3.16)
project(edgelist2metis)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
	definitions.h
	progress.h
	read_edgelist.h
	write_metis.h)

add_executable(edgelist2metis main.cpp ${SOURCE_FILES})
