#include "lib/definitions.h"
#include "lib/mmap_toker.h"
#include "lib/read_obj.h"
#include "lib/utils.h"
#include "lib/write_metis.h"

#include "CLI11.hpp"

using namespace graphtools;

int main(int argc, char* argv[]) {
    std::string input_filename;
    std::string output_filename;

    CLI::App app("obj2metis");
    app.add_option("input mesh", input_filename, "Input mesh")->check(CLI::ExistingFile)->required();
    app.add_option("-o,--output", output_filename, "Output graph");
    CLI11_PARSE(app, argc, argv);

    if (output_filename.empty()) {
        output_filename = build_output_filename(input_filename, kDefaultMetisExtension);
    }

    ID       n = 0;
    EdgeList edges;
    obj::read(
        input_filename, [&] { ++n; },
        [&](const auto v1, const auto v2, const auto v3) {
            edges.emplace_back(v1, v2);
            //edges.emplace_back(v2, v1);
            edges.emplace_back(v2, v3);
            //edges.emplace_back(v3, v2);
            edges.emplace_back(v3, v1);
            //edges.emplace_back(v1, v3);
        }
    );

    sort_edge_list(edges);
    //remove_duplicate_edges(edges);

    metis::write_edge_list(output_filename, edges, n);
}
