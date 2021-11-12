cmake_minimum_required(VERSION 3.16)
project(edgelist2metis)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        buffered_text_output.h
        definitions.h
        mmap_toker.h
        progress.h
        read_edgelist.h
        read_metis.h
        utils.h
        write_binary.h
        write_metis.h)

add_executable(edgelist2metis edgelist2metis.cc ${SOURCE_FILES})
add_executable(metis2binary metis2binary.cc ${SOURCE_FILES})
add_executable(metis2xtrapulp metis2xtrapulp.cc ${SOURCE_FILES})
add_executable(trimmetis trimmetis.cc ${SOURCE_FILES})
add_executable(chkmetis chkmetis.cc ${SOURCE_FILES})
