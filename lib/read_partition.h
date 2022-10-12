#pragma once

#include "definitions.h"
#include "mmap_toker.h"

namespace graphtools::partition {
inline std::vector<ID> read(const std::string& filename, const ID n = 0) {
    MappedFileToker toker(filename);

    std::vector<ID> partition;
    partition.reserve(n);

    while (toker.valid_position()) {
        toker.skip_spaces();
        if (toker.valid_position()) {
            partition.push_back(toker.scan_int<ID>());
        }
        if (toker.valid_position() && toker.current() == '\n') {
            toker.advance();
        } // \n
    }

    return partition;
}
} // namespace graphtools::partition

