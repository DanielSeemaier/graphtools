#include <iostream>

#include "lib/read_metis.h"
#include "lib/utils.h"

#include "CLI11.hpp"

constexpr std::size_t kBufferSize32 = 1024 * 1024 / sizeof(std::uint32_t);

using namespace graphtools;

namespace {
template <typename IDType>
void write_edge_list(std::ofstream& out, const std::vector<IDType>& buffer) {
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(IDType));
}

template <typename IDType, std::size_t kBufferSize>
void convert(const std::string& input_filename, const std::string& output_filename) {
    std::ofstream out(output_filename, std::ios::binary);
    metis::read_edge_list<std::vector<IDType>>(input_filename, kBufferSize, [&](const std::vector<IDType>& buffer) {
        write_edge_list(out, buffer);
    });
}
} // namespace

int main(int argc, char* argv[]) {
    std::string input_filename;
    std::string output_filename;
    bool        use_64_bit_ids = false;

    CLI::App app("metis2xtrapulp");
    app.add_option("input graph", input_filename, "Input graph")->check(CLI::ExistingFile)->required();
    app.add_option("-o,--output", output_filename, "Output graph");
    app.add_flag("--64", use_64_bit_ids, "Use 64 bit IDs");
    CLI11_PARSE(app, argc, argv);

    if (output_filename.empty()) {
        output_filename = build_output_filename(input_filename, kDefaultXtrapulpExtension);
    }

    if (use_64_bit_ids) {
        convert<std::uint64_t, kBufferSize32 / 2>(input_filename, output_filename);
    } else {
        convert<std::uint32_t, kBufferSize32>(input_filename, output_filename);
    }
}
