#pragma once

#include "definitions.h"
#include "mmap_toker.h"

namespace graphtools::obj {
template <typename NodeConsumer, typename FacetConsumer>
inline void read(const std::string& filename, NodeConsumer&& node_consumer, FacetConsumer&& facet_consumer) {
    MappedFileToker toker(filename);

    while (toker.valid_position()) {
        toker.skip_spaces();
        switch (toker.current()) {
            case '\n':
            case '#':
                toker.skip_line();
                break;

            case 'v':
                node_consumer();
                toker.skip_line();
                break;

            case 'f': {
                toker.advance();
                toker.skip_spaces();

                const ID v1 = toker.scan_int<ID>();
                const ID v2 = toker.scan_int<ID>();
                const ID v3 = toker.scan_int<ID>();
                facet_consumer(v1 - 1, v2 - 1, v3 - 1);
                break;
            }

            default:
                // Unexpected symbol
                toker.skip_line();
        }
    }
}
} // namespace graphtools::obj
