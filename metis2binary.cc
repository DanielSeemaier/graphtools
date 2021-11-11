#include "read_metis.h"
#include "utils.h"
#include "write_binary.h"

#include <iostream>
#include <string>
#include <vector>

using namespace graphfmt;

// 128 Mb buffer
constexpr std::size_t kBufferSize = (1024 * 1024) / sizeof(binary::BinaryID);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    std::exit(1);
  }
  const std::string input_filename = argv[1];

  if (!file_exists(input_filename)) {
    std::cout << "input file does not exist" << std::endl;
    std::exit(1);
  }

  const std::string output_filename = build_output_filename(input_filename, "bgf");

  // convert graph
  std::ofstream out(output_filename, std::ios::binary);

  const auto format = metis::read_format(input_filename);
  binary::write_header(out, format.n, format.m);

  using BinaryBuffer = std::vector<binary::BinaryID>;
  metis::read_node_list<BinaryBuffer>(input_filename, kBufferSize, [&](auto &node_list) {
    binary::write_node_list(out, node_list, format.n);
  });
  metis::read_edge_list<BinaryBuffer>(input_filename, kBufferSize, [&](auto &edge_list) {
    binary::write_edge_list(out, edge_list);
  });
}