#pragma once

#include "definitions.h"
#include "mmap_toker.h"

namespace graphtools::gr {
template <typename HeaderConsumer, typename ArcConsumer>
inline void read(const std::string& filename, HeaderConsumer&& header_consumer, ArcConsumer&& arc_consumer) {
    MappedFileToker toker(filename);

    while (toker.valid_position()) {
        toker.skip_spaces();
        switch (toker.current()) {
            case '\n':
            case 'c':
                toker.skip_line();
                break;

            case 'p': {
                toker.consume_string("p sp ");
                const ID n = toker.scan_int<ID>();
                const ID m = toker.scan_int<ID>();
                toker.skip_line();
                header_consumer(n, m);
                break;
            }

            case 'a': {
                toker.advance(); // 'a'
                toker.advance(); // ' '

                const ID u = toker.scan_int<ID>();
                const ID v = toker.scan_int<ID>();
                toker.skip_line(); // weight

                arc_consumer(u - 1, v - 1);
                break;
            }

            default:
                // Unexpected symbol
                toker.skip_line();
        }
    }
}
} // namespace graphtools::gr

