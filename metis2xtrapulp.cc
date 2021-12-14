#include "lib/arguments.h"
#include "lib/read_metis.h"
#include "lib/utils.h"

#include <iostream>

constexpr std::size_t kBufferSize32 = 1024 * 1024 / sizeof(std::uint32_t);

using namespace graphtools;

namespace {
template<typename IDType>
void write_edge_list(std::ofstream &out, const std::vector<IDType> &buffer) {
  out.write(reinterpret_cast<const char *>(buffer.data()), buffer.size() * sizeof(IDType));
}

template<typename IDType, std::size_t kBufferSize>
void convert(const std::string &input_filename, const std::string &output_filename) {
  std::ofstream out(output_filename, std::ios::binary);
  metis::read_edge_list<std::vector<IDType>>(input_filename, kBufferSize,
                                             [&](const std::vector<IDType> &buffer) { write_edge_list(out, buffer); });
}
} // namespace

int main(int argc, char *argv[]) {
  std::string input_filename;
  bool use_64_bit_ids = false;

  { // parse arguments
    Arguments args;
    args.group("Options").argument("64", "Use 64 bit node IDs.", &use_64_bit_ids);
    args.positional().argument("", "Input graph.", &input_filename);
    args.parse(argc, argv);
  }

  const std::string output_filename = build_output_filename(input_filename, "xtrapulp");

  if (!file_exists(input_filename)) {
    std::cout << "input file does not exist" << std::endl;
    std::exit(1);
  }

  if (use_64_bit_ids) {
    convert<std::uint64_t, kBufferSize32 / 2>(input_filename, output_filename);
  } else {
    convert<std::uint32_t, kBufferSize32>(input_filename, output_filename);
  }
}