#pragma once

#include <utility>
#include <vector>

namespace el2metis {
using ID = unsigned long;
using EdgeList = std::vector<std::pair<ID, ID>>;
} // namespace el2metis