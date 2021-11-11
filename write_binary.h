#pragma once

#include "definitions.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace graphfmt::binary {
using BinaryID = unsigned long long;

void write_header(std::ofstream &out, const BinaryID n, const BinaryID m) {
  const BinaryID version = 3;
  out.write(reinterpret_cast<const char *>(&version), sizeof(BinaryID));
  out.write(reinterpret_cast<const char *>(&n), sizeof(BinaryID));
  out.write(reinterpret_cast<const char *>(&m), sizeof(BinaryID));
}

void write_node_list(std::ofstream &out, std::vector<BinaryID> &node_list, const BinaryID n) {
  constexpr BinaryID header_size = 3;
  const BinaryID offset = (header_size + n + 1) * sizeof(BinaryID);

  for (auto &node : node_list) { node = node * sizeof(BinaryID) + offset; }
  out.write(reinterpret_cast<const char *>(node_list.data()),
            static_cast<std::streamsize>(node_list.size() * sizeof(BinaryID)));
}

void write_edge_target_list(std::ofstream &out, const std::vector<BinaryID> &edge_target_list) {
  out.write(reinterpret_cast<const char *>(edge_target_list.data()),
            static_cast<std::streamsize>(edge_target_list.size() * sizeof(BinaryID)));
}
} // namespace graphfmt::binary