#pragma once

#include "definitions.h"
#include "mmap_toker.h"

#include <algorithm>
#include <cctype>

namespace graphtools {
template<typename ProgressLambda>
EdgeList read_edge_list(const std::string &filename, ID &n, ID &m, ProgressLambda &&progress_lambda) {
  MappedFileToker toker(filename);

  toker.advance(); // p
  toker.advance(); // ' '
  n = toker.scan_int<ID>();
  m = toker.scan_int<ID>();
  toker.advance(); // nl

  EdgeList edge_list;
  edge_list.reserve(m);

  std::size_t iteration_counter = 0;
  while (toker.current() == 'e') {
    toker.advance(); // e
    toker.advance(); // ' '
    const auto from = toker.scan_int<ID>() - 1;
    const auto to = toker.scan_int<ID>() - 1;
    toker.advance(); // nl
    edge_list.emplace_back(from, to);

    if (iteration_counter++ >= 1024 * 1024) {
      progress_lambda(toker.position(), toker.length());
      iteration_counter = 0;
    }
  }

  auto edge_comparator = [&](const auto &lhs, const auto &rhs) {
    return lhs.first < rhs.first;
  };

  if (!std::is_sorted(edge_list.begin(), edge_list.end(), edge_comparator)) {
    std::sort(edge_list.begin(), edge_list.end(), edge_comparator);
  }
  return edge_list;
}
} // namespace el2metis
