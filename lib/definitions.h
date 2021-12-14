#pragma once

#include <utility>
#include <vector>

namespace graphtools {
using ID = unsigned long;
using Weight = signed long;
using EdgeList = std::vector<std::pair<ID, ID>>;
} // namespace el2metis