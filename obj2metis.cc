#include "lib/mmap_toker.h"

#include "CLI11.hpp"

int main(int argc, char* argv[]) {
    std::string input_filename;
    std::string output_filename;

    CLI::App app("obj2metis");
    app.add_option("input mesh", input_filename, "Input mesh")->check(CLI::ExistingFile)->required();
    app.add_option("-o,--output", output_filename, "Output graph");
    CLI11_PARSE(app, argc, argv);


}
