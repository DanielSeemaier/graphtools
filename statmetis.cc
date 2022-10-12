#include "CLI11.hpp"
#include "lib/read_metis.h"

#include <iostream>

using namespace graphtools;

struct Statistics {
  std::string graph{};
  ID n{};
  ID m{};
};

void print_csv_header(const bool /* fast */) { std::cout << "Graph,N,M\n"; }

void print_csv_row(const Statistics &stats, const bool /* fast */) {
  std::cout << stats.graph << "," << stats.n << "," << stats.m << "\n";
}

void print_verbose(const Statistics &stats, const bool /* fast */) {
  std::cout << "Graph: " << stats.graph << "\n"
            << "Number of nodes: " << stats.n << "\n"
            << "Number of edges: " << stats.m << "\n";
}

int main(int argc, char *argv[]) {
  bool csv = false;
  bool csv_header = false;
  bool fast = false;
  std::string filename;

  CLI::App app("statmetis");
  app.add_option("input graph", filename, "Input graph")->check(CLI::ExistingFile);
  app.add_flag("-f,--fast", fast);
  app.add_flag("-c,--csv", csv);
  app.add_flag("-h,--csv-header", csv_header);
  CLI11_PARSE(app, argc, argv);

  if (!csv_header && filename.empty()) {
    std::cout << "invalid usage; specify graph filename!" << std::endl;
    std::exit(1);
  }

  // Print CSV header if requested
  if (csv_header) { print_csv_header(fast); }

  // Exit if there is no graph
  // This is useful if we only want the CSV header
  if (filename.empty()) { std::exit(0); }

  MappedFileToker toker(filename);
  const auto [n, m, has_node_weights, has_edge_weights] = metis::read_format(toker);

  Statistics stats;
  stats.graph = filename;
  stats.n = n;
  stats.m = m;

  if (csv) {
    print_csv_row(stats, fast);
  } else {
    print_verbose(stats, fast);
  }
}

