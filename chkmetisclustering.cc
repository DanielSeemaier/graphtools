#include <algorithm>
#include <iostream>
#include <set>
#include <string>

#include "lib/read_metis.h"
#include "lib/read_partition.h"

#include "CLI11.hpp"

using namespace graphtools;

int main(int argc, char* argv[]) {
    std::string graph_filename;
    std::string clustering_filename;

    CLI::App app("chkmetisclustering");
    app.add_option("input graph", graph_filename, "Input graph")->check(CLI::ExistingFile)->required();
    app.add_option("input clustering", clustering_filename, "Input clustering")->check(CLI::ExistingFile)->required();
    CLI11_PARSE(app, argc, argv);

    // Load partition
    const auto clustering = partition::read(clustering_filename);

    // Load graph
    MappedFileToker graph_toker(graph_filename);
    const auto      format = metis::read_format(graph_toker);

    // Make sure that the size of the clustering matches the size of the graph
    if (clustering.size() < format.n) {
        std::cerr << "Error: graph has " << format.n << " nodes, but clustering file has only " << clustering.size()
                  << " lines\n";
        std::exit(1);
    } else if (clustering.size() != format.n) {
        std::cerr << "Warning: graph has " << format.n << " nodes, but partition file has " << clustering.size()
                  << " lines\n";
    }
    for (ID u = 0; u < format.n; ++u) {
        if (clustering[u] >= format.n) {
            std::cerr << "Error: node " << u << " is in cluster " << clustering[u] << ", which is out-of-bounds\n";
            std::exit(1);
        }
    }

    // Count cluster sizes and edge cuts
    std::vector<Weight> cluster_weights(format.n);
    Weight              edge_cut          = 0;
    Weight              total_node_weight = 0;

    metis::read_graph_weighted(
        graph_filename,
        [&](const ID u, const Weight weight) {
            cluster_weights[clustering[u]] += weight;
            total_node_weight += weight;
        },
        [&](const ID u, const ID v, const Weight weight) {
            if (weight != 1) {
                std::cerr << weight << "\n";
            }
            if (clustering[u] != clustering[v]) {
                edge_cut += weight;
            }
        }
    );

    // Generate cluster statistics
    const ID number_of_clusters =
        std::count_if(cluster_weights.begin(), cluster_weights.end(), [&](const Weight weight) { return weight > 0; });
    const Weight max_weight = *std::max_element(cluster_weights.begin(), cluster_weights.end());

    std::cout << "cut=" << edge_cut << " num_clusters=" << number_of_clusters << " max_weight=" << max_weight
              << std::endl;
}

