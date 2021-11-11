#pragma once

#include <fstream>
#include <string>

namespace graphfmt {
bool file_exists(const std::string &filename) {
  std::ifstream in(filename);
  return in.good();
}

std::string build_output_filename(const std::string &input_filename, const std::string &output_extension) {
  const auto dot_pos = input_filename.find_last_of('.');
  if (dot_pos == std::string::npos) { return input_filename + "." + output_extension; }
  return input_filename.substr(0, dot_pos) + "." + output_extension;
}
} // namespace graphfmt