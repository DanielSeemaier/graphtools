#include "lib/arguments.h"
#include "lib/read_metis.h"

#include <iostream>

using namespace graphtools;

struct Statistics {
  std::string graph{};
  ID n{};
  ID m{};
  ID max_degree{};
};

void print_csv_header(const bool /* fast */) { std::cout << "Graph,N,M,MaxDegree\n"; }

void print_csv_row(const Statistics &stats, const bool /* fast */) {
  std::cout << stats.graph << "," << stats.n << "," << stats.m << "," << stats.max_degree << "\n";
}

void print_verbose(const Statistics &stats, const bool /* fast */) {
  std::cout << "Graph: " << stats.graph << "\n"
            << "Number of nodes: " << stats.n << "\n"
            << "Number of edges: " << stats.m << "\n"
            << "Maximum degree:  " << stats.max_degree << "\n";
}

int main(int argc, char *argv[]) {
  bool csv = false;
  bool csv_header = false;
  bool fast = false;
  std::string filename;

  Arguments args;
  args.positional().argument("graph", "Graph filename", &filename, 'G');
  args.group("General options")
      .argument("fast", "Only print statistics that can be computed without reading the entire graph.", &fast, 'f');
  args.group("Output options")
      .argument("csv", "Use CSV format.", &csv, 'c')
      .argument("header", "Print CSV header.", &csv_header, 'h');
  args.parse(argc, argv);

  if (!csv_header && filename.empty()) {
    std::cout << "invalid usage; specify graph filename!" << std::endl;
    std::exit(1);
  }

  // print CSV header if requested
  if (csv_header) { print_csv_header(fast); }

  // exit if there is no graph
  // this is useful if we only want to CSV header
  if (filename.empty()) { std::exit(0); }

  MappedFileToker toker(filename);
  const auto [n, m, has_node_weights, has_edge_weights] = metis::read_format(toker);


  ID max_degree = 0;
  ID current_node = 0;
  ID current_degree = 0;
  metis::read_graph(filename, [&](const ID u, const ID v) {
    if (u != current_node) {
      current_node = u;
      current_degree = 0;
    }
    current_degree++;
    max_degree = std::max(current_degree, max_degree);
  });

  Statistics stats;
  stats.graph = filename;
  stats.n = n;
  stats.m = m;
  stats.max_degree = max_degree;

  if (csv) {
    print_csv_row(stats, fast);
  } else {
    print_verbose(stats, fast);
  }
}
