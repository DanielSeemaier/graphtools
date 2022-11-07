#pragma once

#include <limits>
#include <type_traits>
#include <vector>

#include "definitions.h"
#include "mmap_toker.h"

namespace graphtools::metis {
struct Format {
    ID   n;
    ID   m;
    bool has_node_weights;
    bool has_edge_weights;
};

inline Format read_format(MappedFileToker& toker) {
    toker.skip_spaces();
    while (toker.current() == '%') {
        toker.skip_line();
        toker.skip_spaces();
    }

    const ID n      = toker.scan_int<ID>();
    const ID m      = toker.scan_int<ID>() * 2;
    const ID format = std::isdigit(toker.current()) ? toker.scan_int<ID>() : 0;
    if (toker.current() == '\n') {
        toker.advance();
    }
    return {n, m, static_cast<bool>(format / 10), static_cast<bool>(format % 10)};
}

inline Format read_format(const std::string& filename) {
    MappedFileToker toker(filename);
    return read_format(toker);
}

//! Read node degrees without parsing numbers.
template <typename Buffer, typename NodeListConsumer>
void read_node_list(const std::string& filename, const std::size_t buffer_size, NodeListConsumer&& consumer) {
    MappedFileToker toker(filename);
    const auto [n, m, has_node_weights, has_edge_weights] = read_format(toker);

    Buffer node_list;
    node_list.reserve(buffer_size);

    ID cur_chunk = 0;
    ID offset    = 0;

    while (cur_chunk < n) {
        for (ID cur_u = cur_chunk; cur_u < std::min(n, cur_chunk + buffer_size); ++cur_u) {
            toker.skip_spaces();
            while (toker.current() == '%') {
                toker.skip_line();
                toker.skip_spaces();
            }

            node_list.push_back(offset);
            if (has_node_weights) {
                toker.skip_int();
            }

            while (std::isdigit(toker.current())) {
                toker.skip_int();
                if (has_edge_weights) {
                    toker.skip_int();
                }
                ++offset;
            }

            if (toker.current() == '\n') {
                toker.advance();
            }
        }

        cur_chunk += buffer_size;
        consumer(node_list);
        node_list.clear();
    }

    node_list.push_back(offset);
    consumer(node_list);
}

template <typename NodeConsumer, typename EdgeConsumer>
void read_graph_weighted(const std::string& filename, NodeConsumer&& node_consumer, EdgeConsumer&& edge_consumer) {
    MappedFileToker toker(filename);
    const auto [n, m, has_node_weights, has_edge_weights] = read_format(toker);

    for (ID u = 0; u < n; ++u) {
        toker.skip_spaces();
        while (toker.current() == '%') {
            toker.skip_line();
            toker.skip_spaces();
        }

        Weight weight_u = 1;
        if (has_node_weights) {
            weight_u = toker.scan_int<Weight>();
        }
        node_consumer(u, weight_u);

        while (std::isdigit(toker.current())) {
            const ID v      = toker.scan_int<ID>() - 1;
            Weight   weight = 1;
            if (has_edge_weights) {
                weight = toker.scan_int<Weight>();
            }
            edge_consumer(u, v, weight);
        }

        if (toker.current() == '\n') {
            toker.advance();
        }
    }
}

//! Read graph and call consumer lambda on each edge.
template <typename Consumer>
void read_graph(MappedFileToker& toker, Consumer&& consumer) {
    const auto [n, m, has_node_weights, has_edge_weights] = read_format(toker);

    for (ID u = 0; u < n; ++u) {
        toker.skip_spaces();
        while (toker.current() == '%') {
            toker.skip_line();
            toker.skip_spaces();
        }

        if (has_node_weights) {
            toker.skip_int();
        }
        while (std::isdigit(toker.current())) {
            const ID v = toker.scan_int<ID>() - 1;
            if (has_edge_weights) {
                toker.skip_int();
            }
            consumer(u, v);
        }

        if (toker.current() == '\n') {
            toker.advance();
        }
    }
}

template <typename Consumer>
void read_graph(const std::string& filename, Consumer&& consumer) {
    MappedFileToker toker{filename};
    read_graph(toker, std::forward<Consumer>(consumer));
}

//! Read **target of edges** into buffer, call consumer whenever buffer is full.
template <typename Buffer, typename EdgeTargetListConsumer>
void read_edge_target_list(
    const std::string& filename, const std::size_t buffer_size, EdgeTargetListConsumer&& consumer
) {
    Buffer edge_target_buffer;
    edge_target_buffer.reserve(buffer_size);
    read_graph(filename, [&](const ID, const ID to) {
        edge_target_buffer.push_back(to);
        if (edge_target_buffer.size() == buffer_size) {
            consumer(edge_target_buffer);
            edge_target_buffer.clear();
            edge_target_buffer.reserve(buffer_size);
        }
    });

    consumer(edge_target_buffer);
}

//! Read **source and target of edges** into buffer, call consumer whenever buffer is full.
template <typename Buffer, typename EdgeListConsumer>
void read_edge_list(const std::string& filename, const std::size_t buffer_size, EdgeListConsumer&& consumer) {
    const std::size_t actual_buffer_size = buffer_size + (buffer_size & 1); // make buffer size even

    Buffer edge_list;
    edge_list.reserve(actual_buffer_size);

    read_graph(filename, [&](const ID from, const ID to) {
        edge_list.push_back(from);
        edge_list.push_back(to);

        if (edge_list.size() == actual_buffer_size) {
            consumer(edge_list);
            edge_list.clear();
            edge_list.reserve(actual_buffer_size);
        }
    });

    consumer(edge_list);
}
} // namespace graphtools::metis

