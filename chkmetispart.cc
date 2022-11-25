#include <algorithm>
#include <iostream>
#include <string>

#include "lib/read_metis.h"
#include "lib/read_partition.h"

#include "CLI11.hpp"

using namespace graphtools;

int main(int argc, char* argv[]) {
    std::string graph_filename;
    std::string partition_filename;

    CLI::App app("chkmetispart");
    app.add_option("input graph", graph_filename, "Input graph")->check(CLI::ExistingFile)->required();
    app.add_option("input partition", partition_filename, "Input partition")->check(CLI::ExistingFile)->required();
    CLI11_PARSE(app, argc, argv);

    // load partition
    const auto partition = partition::read(partition_filename);
    const ID   k         = partition.empty() ? 0 : (*std::max_element(partition.begin(), partition.end())) + 1;

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
    ID              cut = 0;

    metis::read_graph_weighted(
        graph_filename, [&](const ID u, const Weight weight) { block_sizes[partition[u]] += weight; },
        [&](const ID u, const ID v, const Weight weight) { cut += weight * (partition[u] != partition[v]); }
    );

    const ID     max_weight = *std::max_element(block_sizes.begin(), block_sizes.end());
    const double imbalance  = static_cast<double>(max_weight) / (static_cast<double>(n) / static_cast<double>(k));

    std::cout << "cut=" << (cut / 2) << " imbalance=" << imbalance << std::endl;
}
