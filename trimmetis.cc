#include "lib/read_metis.h"
#include "lib/utils.h"
#include "lib/write_metis.h"

#include <iostream>
#include <string>

using namespace graphtools;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << std::endl;
    std::exit(1);
  }

  const std::string input_filename = argv[1];
  if (!file_exists(input_filename)) {
    std::cout << "input filename does not exist" << std::endl;
    std::exit(1);
  }

  const std::string output_filename = build_output_filename(input_filename, "graph.trimmed");

  BufferedTextOutput out{tag::create, output_filename};

  const auto [n, m, has_node_weights, has_edge_weights] = metis::read_format(input_filename);
  metis::write_format(out, n, m);

  ID last_node = 0;
  metis::read_graph(input_filename, [&](const ID from, const ID to) {
    //std::cout << from << " --> " << to << std::endl;
    //std::exit(0);
    metis::write_edge(out, last_node, from, to);
    last_node = from;
  });
  metis::write_finish(out, last_node, n);
}