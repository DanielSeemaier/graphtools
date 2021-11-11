#pragma once

#include "definitions.h"
#include "mmap_toker.h"

#include <limits>
#include <type_traits>
#include <vector>

namespace graphfmt::metis {
struct Format {
  ID n;
  ID m;
  bool has_node_weights;
  bool has_edge_weights;
};

Format read_format(MappedFileToker &toker) {
  toker.skip_spaces();
  while (toker.current() == '%') {
    toker.skip_line();
    toker.skip_spaces();
  }

  const ID n = toker.scan_int<ID>();
  const ID m = toker.scan_int<ID>() * 2;
  const ID format = std::isdigit(toker.current()) ? toker.scan_int<ID>() : 0;
  if (toker.current() == '\n') { toker.advance(); }
  return {n, m, static_cast<bool>(format / 10), static_cast<bool>(format % 10)};
}

Format read_format(const std::string &filename) {
  MappedFileToker toker(filename);
  return read_format(toker);
}

template<typename Buffer, typename NodeListConsumer>
void read_node_list(const std::string &filename, const std::size_t buffer_size, NodeListConsumer &&consumer) {
  MappedFileToker toker(filename);
  const auto [n, m, has_node_weights, has_edge_weights] = read_format(toker);

  Buffer node_list;
  node_list.reserve(buffer_size);

  ID cur_chunk = 0;
  ID offset = 0;

  while (cur_chunk < n) {
    for (ID cur_u = cur_chunk; cur_u < std::min(n, cur_chunk + buffer_size); ++cur_u) {
      toker.skip_spaces();
      while (toker.current() == '%') {
        toker.skip_line();
        toker.skip_spaces();
      }

      node_list.push_back(offset);
      if (has_node_weights) { toker.skip_int(); }

      while (std::isdigit(toker.current())) {
        if (has_edge_weights) { toker.skip_int(); }
        toker.skip_int();
        ++offset;
      }

      if (toker.current() == '\n') { toker.advance(); }
    }

    cur_chunk += buffer_size;
    consumer(node_list);
    node_list.clear();
  }

  node_list.push_back(offset);
  consumer(node_list);
}

template<typename Buffer, typename EdgeListConsumer>
void read_edge_list(const std::string &filename, const std::size_t buffer_size, EdgeListConsumer &&consumer) {
  MappedFileToker toker(filename);
  const auto [n, m, has_node_weights, has_edge_weights] = read_format(toker);

  Buffer edge_list;
  edge_list.reserve(buffer_size);

  for (ID cur_e = 0; cur_e < m; ++cur_e) {
    toker.skip_spaces();
    while (toker.current() == '%') {
      toker.skip_line();
      toker.skip_spaces();
    }

    if (has_node_weights) { toker.skip_int(); }
    while (std::isdigit(toker.current())) {
      if (has_edge_weights) { toker.skip_int(); }
      edge_list.push_back(toker.scan_int<ID>() - 1);

      if (edge_list.size() == buffer_size) {
        consumer(edge_list);
        edge_list.clear();
      }
    }

    if (toker.current() == '\n') { toker.advance(); }
  }

  consumer(edge_list);
}
} // namespace graphfmt::metis