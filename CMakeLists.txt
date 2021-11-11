cmake_minimum_required(VERSION 3.16)
project(edgelist2metis)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
	definitions.h
	progress.h
	read_edgelist.h
	write_metis.h)

add_library(libgraphfmt INTERFACE
		arguments.h
		definitions.h
		utils.h
		mmap_toker.h
		progress.h
		read_edgelist.h
		read_metis.h
		write_metis.h
		write_binary.h)

add_executable(edgelist2metis edgelist2metis.cc)
target_link_libraries(edgelist2metis INTERFACE libgraphfmt)
add_executable(metis2binary metis2binary.cc)
target_link_libraries(metis2binary INTERFACE libgraphfmt)
add_executable(chkmetis chkmetis.cc)
target_link_libraries(chkmetis INTERFACE libgraphfmt)
add_executable(metis2xtrapulp metis2xtrapulp.cc)
target_link_libraries(metis2xtrapulp INTERFACE libgraphfmt)