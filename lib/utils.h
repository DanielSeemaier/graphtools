#pragma once

#include <algorithm>
#include <fstream>
#include <string>

#include "definitions.h"

namespace graphtools {
inline bool file_exists(const std::string& filename) {
    std::ifstream in(filename);
    return in.good();
}

inline std::string build_output_filename(const std::string& input_filename, const std::string& output_extension) {
    const auto  dot_pos         = input_filename.find_last_of('.');
    std::string output_filename = dot_pos == std::string::npos
                                      ? input_filename + "." + output_extension
                                      : input_filename.substr(0, dot_pos) + "." + output_extension;
    if (output_filename == input_filename) {
        return input_filename + ".2";
    }
    return output_filename;
}

inline void sort_edge_list(EdgeList& edges) {
    if (!std::is_sorted(edges.begin(), edges.end())) {
        std::sort(edges.begin(), edges.end());
    }
}

inline void remove_duplicate_edges(EdgeList& edges) {
    auto it = std::unique(edges.begin(), edges.end());
    edges.erase(it, edges.end());
}
} // namespace graphtools

