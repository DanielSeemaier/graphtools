#include "CLI11.hpp"
#include "lib/read_metis.h"
#include "lib/utils.h"
#include "lib/write_binary.h"

#include <iostream>
#include <string>
#include <vector>

using namespace graphtools;

// 1 Mb buffer
constexpr std::size_t kBufferSize = (1024 * 1024) / sizeof(binary::BinaryID);

int main(int argc, char *argv[]) {
  std::string input_filename;
  std::string output_filename;

  CLI::App app("metis2binary");
  app.add_option("input graph", input_filename, "Input graph")->check(CLI::ExistingFile)->required();
  app.add_option("-o,--output", output_filename, "Output graph");
  CLI11_PARSE(app, argc, argv);

  if (!file_exists(input_filename)) {
    std::cout << "input file does not exist" << std::endl;
    std::exit(1);
  }

  if (output_filename.empty()) { output_filename = build_output_filename(input_filename, "bgf"); }

  // convert graph
  std::ofstream out(output_filename, std::ios::binary);

  const auto format = metis::read_format(input_filename);
  binary::write_header(out, format.n, format.m);

  using BinaryBuffer = std::vector<binary::BinaryID>;
  metis::read_node_list<BinaryBuffer>(input_filename, kBufferSize,
                                      [&](auto &node_list) { binary::write_node_list(out, node_list, format.n); });
  metis::read_edge_target_list<BinaryBuffer>(input_filename, kBufferSize,
                                             [&](auto &edge_list) { binary::write_edge_target_list(out, edge_list); });
}
