#include "CLI11.hpp"
#include "lib/read_metis.h"
#include "lib/read_partition.h"

#include <algorithm>
#include <iostream>
#include <string>

using namespace graphtools;

int main(int argc, char *argv[]) {
  std::string graph_filename;
  std::string partition_filename;

  CLI::App app("chkmetispart");
  app.add_option("input graph", graph_filename, "Input graph")->check(CLI::ExistingFile)->required();
  app.add_option("input partition", partition_filename, "Input partition")->check(CLI::ExistingFile)->required();
  CLI11_PARSE(app, argc, argv);

  // load partition
  const auto partition = partition::read(partition_filename);
  const ID k = partition.empty() ? 0 : (*std::max_element(partition.begin(), partition.end())) + 1;

  // scan graph
  MappedFileToker graph_toker(graph_filename);
  const auto [n, m, has_node_weights, has_edge_weights] = metis::read_format(graph_toker);

  if (partition.size() < n) {
    std::cout << "error: graph has " << n << " nodes, but partition file has only " << partition.size() << " lines"
              << std::endl;
    std::exit(1);
  } else if (partition.size() != n) {
    std::cout << "warning: graph has " << n << " nodes, but partition file has " << partition.size() << " lines"
              << std::endl;
  }

  if (k == 0) { // special case: empty graph
    std::cout << "cut=0 imbalance=1" << std::endl;
    std::exit(0);
  }

  std::vector<ID> block_sizes(k);
  for (ID u = 0; u < n; ++u) { ++block_sizes[partition[u]]; }
  const ID max_weight = *std::max_element(block_sizes.begin(), block_sizes.end());
  const double imbalance = static_cast<double>(max_weight) / (static_cast<double>(n) / static_cast<double>(k));

  ID cut = 0;
  metis::read_graph(graph_toker, [&](const ID u, const ID v) { cut += partition[u] != partition[v]; });

  std::cout << "cut=" << cut << " imbalance=" << imbalance << std::endl;
}
