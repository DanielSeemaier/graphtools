#include "lib/definitions.h"
#include "lib/mmap_toker.h"
#include "lib/read_stp.h"
#include "lib/utils.h"
#include "lib/write_metis.h"

#include "CLI11.hpp"

using namespace graphtools;

int main(int argc, char* argv[]) {
    std::string input_filename;
    std::string output_filename;

    CLI::App app("stp2metis");
    app.add_option("input mesh", input_filename, "Input mesh")->check(CLI::ExistingFile)->required();
    app.add_option("-o,--output", output_filename, "Output graph");
    CLI11_PARSE(app, argc, argv);

    if (output_filename.empty()) {
        output_filename = build_output_filename(input_filename, kDefaultMetisExtension);
    }

    ID       graph_n = 0;
    EdgeList edges;
    stp::read(
        input_filename, [&](const ID n, auto) { graph_n = n; },
        [&](const ID u, const ID v) {
            edges.emplace_back(u, v);
            edges.emplace_back(v, u);
        }
    );

    sort_edge_list(edges);
    remove_duplicate_edges(edges);

    metis::write_edge_list(output_filename, edges, graph_n);
}
