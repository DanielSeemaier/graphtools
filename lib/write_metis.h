#pragma once

#include "buffered_text_output.h"
#include "definitions.h"
#include "mmap_toker.h"
#include "read_edgelist.h"

namespace graphtools::metis {
inline void write_format(
    BufferedTextOutput<>& out, const ID n, const ID m, const bool node_weights = false, const bool edge_weights = false
) {
    out.write_int(n).write_char(' ').write_int(m / 2);
    if (node_weights || edge_weights) {
        out.write_char(' ');
        if (node_weights) {
            out.write_int(node_weights).write_int(edge_weights);
        } else {
            out.write_int(edge_weights);
        }
    }
    out.write_char('\n');
    out.flush();
}

inline void write_format(
    const std::string& output_filename, const ID n, const ID m, const bool node_weights = false,
    const bool edge_weights = false
) {
    BufferedTextOutput out{tag::create, output_filename};
    write_format(out, n, m, node_weights, edge_weights);
}

template <typename ProgressLambda>
void write_graph_part(
    const std::string& filename, const EdgeList& edge_list, const ID from, const ID to, ProgressLambda&& progress_lambda
) {
    BufferedTextOutput out(tag::append, filename);

    ID          cur_u             = from;
    ID          edges_written     = 0;
    std::size_t iteration_counter = 0;

    for (const auto& [u, v]: edge_list) {
        while (u > cur_u) {
            out.write_char('\n').flush();
            ++cur_u;
        }

        out.write_int(v + 1).write_char(' ').flush();
        ++edges_written;

        if (iteration_counter++ >= 1024 * 1024) {
            progress_lambda(edges_written, edge_list.size());
            iteration_counter = 0;
        }
    }

    // finish last node
    if (!edge_list.empty()) {
        ++cur_u;
        out.write_char('\n');
    }

    // add isolated nodes
    while (to > cur_u) {
        out.write_char('\n').flush();
        ++cur_u;
    }
}

inline void write_graph_part(const std::string& filename, const EdgeList& edge_list, const ID from, const ID to) {
    write_graph_part(filename, edge_list, from, to, [&](auto, auto) {});
}

inline void write_edge(BufferedTextOutput<>& out, const ID last_from, const ID from, const ID to) {
    for (ID cur_u = last_from; cur_u < from; ++cur_u) {
        out.write_char('\n').flush();
    }
    out.write_int(to + 1).write_char(' ').flush();
}

inline void write_finish(BufferedTextOutput<>& out, const ID last_from, const ID n) {
    for (ID cur_u = last_from; cur_u < n; ++cur_u) {
        out.write_char('\n').flush();
    }
}

inline void write_finish(const std::string& filename, const ID last_from, const ID n) {
    BufferedTextOutput out(tag::append, filename);
    write_finish(out, last_from, n);
}

inline void write_edge_list(const std::string& filename, const EdgeList& edge_list, const ID n) {
    write_format(filename, n, edge_list.size());
    write_graph_part(filename, edge_list, 0, n);
    write_finish(filename, edge_list.back().first + 1, n);
}
} // namespace graphtools::metis

