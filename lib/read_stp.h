#pragma once

#include "definitions.h"
#include "mmap_toker.h"

namespace graphtools::stp {
template <typename HeaderConsumer, typename ArcConsumer>
inline void read(const std::string& filename, HeaderConsumer&& header_consumer, ArcConsumer&& arc_consumer) {
    MappedFileToker toker(filename);

    bool in_graph_section = false;

    while (toker.valid_position()) {
        toker.skip_spaces();
        if (in_graph_section) {
            if (toker.test_string("End")) {
                break;
            }

            if (toker.current() == 'E') {
                toker.advance(); // 'E'
                toker.advance(); // ' '
                const ID u = toker.scan_int<ID>();
                const ID v = toker.scan_int<ID>();
                arc_consumer(u - 1, v - 1);
            } // else -> ???
            toker.skip_line();
        } else {
            if (toker.test_string("Section Graph")) {
                toker.skip_line();
                toker.consume_string("Nodes ");
                const ID n = toker.scan_int<ID>();
                toker.skip_line();
                toker.consume_string("Edges ");
                const ID m       = toker.scan_int<ID>();
                in_graph_section = true;

                header_consumer(n, m);
            }
            toker.skip_line();
        }
    }
}
} // namespace graphtools::stp
