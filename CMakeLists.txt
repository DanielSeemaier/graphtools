cmake_minimum_required(VERSION 3.16)
project(edgelist2metis)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        lib/buffered_text_output.h
        lib/definitions.h
        lib/mmap_toker.h
        lib/progress.h
        lib/read_edgelist.h
        lib/read_metis.h
        lib/read_partition.h
        lib/utils.h
        lib/write_binary.h
        lib/write_metis.h)

add_executable(edgelist2metis edgelist2metis.cc ${SOURCE_FILES})
add_executable(metis2binary metis2binary.cc ${SOURCE_FILES})
add_executable(metis2xtrapulp metis2xtrapulp.cc ${SOURCE_FILES})
add_executable(trimmetis trimmetis.cc ${SOURCE_FILES})
add_executable(chkmetis chkmetis.cc ${SOURCE_FILES})
add_executable(chkmetispart chkmetispart.cc ${SOURCE_FILES})
