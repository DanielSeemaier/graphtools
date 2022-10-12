cmake_minimum_required(VERSION 3.16)
project(graphtools)

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

# x to METIS
add_executable(edgelist2metis edgelist2metis.cc ${SOURCE_FILES})
install(TARGETS edgelist2metis DESTINATION bin)

add_executable(obj2metis obj2metis.cc ${SOURCE_FILES})
install(TARGETS obj2metis DESTINATION bin)

# METIS to x
add_executable(metis2binary metis2binary.cc ${SOURCE_FILES})
install(TARGETS metis2binary DESTINATION bin)

add_executable(metis2xtrapulp metis2xtrapulp.cc ${SOURCE_FILES})
install(TARGETS metis2xtrapulp DESTINATION bin)

# METIS graph format
add_executable(trimmetis trimmetis.cc ${SOURCE_FILES})
install(TARGETS trimmetis DESTINATION bin)

add_executable(chkmetis chkmetis.cc ${SOURCE_FILES})
install(TARGETS chkmetis DESTINATION bin)

add_executable(chkmetispart chkmetispart.cc ${SOURCE_FILES})
install(TARGETS chkmetispart DESTINATION bin)

add_executable(statmetis statmetis.cc ${SOURCE_FILES})
install(TARGETS statmetis DESTINATION bin)
