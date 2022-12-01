#include <iostream>
#include <string>

#include "lib/definitions.h"
#include "lib/read_metis.h"
#include "lib/utils.h"
#include "lib/write_metis.h"

#include "CLI11.hpp"

using namespace graphtools;

int main(int argc, char* argv[]) {
    std::string input_filename;
    std::string output_filename;

    CLI::App app("trimmetis");
    app.add_option("input graph", input_filename, "Input graph")->check(CLI::ExistingFile)->required();
    app.add_option("-o,--output", output_filename, "Output graph");
    CLI11_PARSE(app, argc, argv);

    if (output_filename.empty()) {
        output_filename = build_output_filename(input_filename, kDefaultTrimmedMetisExtension);
    }

    BufferedTextOutput<> out{tag::create, output_filename};

    const auto [n, m, has_node_weights, has_edge_weights] = metis::read_format(input_filename);
    metis::write_format(out, n, m);

    ID last_node = 0;
    metis::read_graph(input_filename, [&](const ID from, const ID to) {
        metis::write_edge(out, last_node, from, to);
        last_node = from;
    });
    metis::write_finish(out, last_node, n);
}

