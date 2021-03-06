#pragma once

#include <fstream>
#include <string>

namespace graphtools {
bool file_exists(const std::string &filename) {
  std::ifstream in(filename);
  return in.good();
}

std::string build_output_filename(const std::string &input_filename, const std::string &output_extension) {
  const auto dot_pos = input_filename.find_last_of('.');
  std::string output_filename = dot_pos == std::string::npos
                                    ? input_filename + "." + output_extension
                                    : input_filename.substr(0, dot_pos) + "." + output_extension;
  if (output_filename == input_filename) { return input_filename + ".2"; }
  return output_filename;
}
} // namespace graphfmt