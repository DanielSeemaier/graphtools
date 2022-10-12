#pragma once

#include <utility>
#include <vector>

namespace graphtools {
using ID       = unsigned long;
using Weight   = signed long;
using EdgeList = std::vector<std::pair<ID, ID>>;

constexpr const char* kDefaultMetisExtension        = "graph";
constexpr const char* kDefaultTrimmedMetisExtension = "graph.trimmed";
constexpr const char* kDefaultBinaryExtension       = "bgf";
constexpr const char* kDefaultXtrapulpExtension     = "xtrapulp";
} // namespace graphtools

