#include "lib/definitions.h"
#include "lib/progress.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace graphtools;

namespace {
using idx = signed long long;

struct Edge {
  idx u;
  idx v;
  idx weight;

  bool operator<(const idx other_v) const { return v < other_v; }
  bool operator<=(const idx other_v) const { return v <= other_v; }
};

template<typename ForwardIterator, typename T>
ForwardIterator binary_find(ForwardIterator begin, ForwardIterator end, const T &val) {
  const auto i = std::lower_bound(begin, end, val);
  return (i != end && (*i <= val)) ? i : end;
}

bool is_comment_line(const std::string &line) {
  const auto first_letter = line.find_first_not_of(' ');
  return first_letter != std::string::npos && line[first_letter] == '%';
}
} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    std::exit(1);
  }

  const std::string graph_filename{argv[1]};

  std::ifstream in(graph_filename);
  std::string line;
  while (std::getline(in, line) && is_comment_line(line)) {}

  idx n{0};
  idx m{0};
  idx format{0};
  std::stringstream(line) >> n >> m >> format;

  if (n < 0) {
    std::cout << "number of nodes cannot be negative: " << n << " (stopping)" << std::endl;
    std::exit(1);
  }
  if (m < 0) {
    std::cout << "number of edges cannot be negative: " << m << " (stopping)" << std::endl;
    std::exit(1);
  }
  if (m > n * (n - 1) / 2) {
    std::cout << "there are too many edges in the graph: with " << n << " nodes, there can eb at most "
              << (n * (n - 1) / 2) << " undirected edges in the graph, but there are " << m << " undirected edges "
              << "(stopping)" << std::endl;
    std::exit(1);
  }
  if (n > std::numeric_limits<ID>::max() || m * 2 > std::numeric_limits<ID>::max()) {
    std::cout << "to load this graph, you must increase the width of the ID datatype (stopping)" << std::endl;
    std::exit(1);
  }

  if (format != 0 && format != 1 && format != 10 && format != 11) {
    std::cout << "graph format " << format << " is unsupported, should be 0, 1, 10 or 11 (stopping)" << std::endl;
    std::exit(1);
  }

  const bool has_node_weights = format / 10;
  const bool has_edge_weights = format % 10;
  m *= 2;

  std::vector<Edge> edges;
  edges.reserve(m);
  idx total_node_weight{0};
  idx total_edge_weight{0};

  idx cur_u = 0;
  while (std::getline(in, line)) {
    if (is_comment_line(line)) { continue; }
    std::stringstream node(line);

    if (has_node_weights) {
      idx weight;
      node >> weight;
      total_node_weight += weight;

      if (weight < 0) {
        std::cout << "node weight of node " << cur_u + 1 << " cannot be negative: " << weight << std::endl;
      } else if (weight > std::numeric_limits<Weight>::max()) {
        std::cout << "to load this graph, you must increase the width of the Weight datatype due to the node weight of "
                  << "node " << cur_u + 1 << ": " << weight << std::endl;
      }
    }

    idx v;
    while (node >> v) {
      --v;

      if (v < 0) {
        std::cout << "invalid node in the neighborhood of node " << cur_u + 1 << ": " << v + 1 << " "
                  << "must be greater than 0" << std::endl;
      } else if (v >= n) {
        std::cout << "invalid node in the neighborhood of node " << cur_u + 1 << ": " << v + 1 << " "
                  << "is higher than the number of nodes" << std::endl;
      } else if (v == cur_u) {
        std::cout << "graph contains a self-loop on node " << cur_u + 1 << std::endl;
      }

      idx weight{1};
      if (has_edge_weights) {
        node >> weight;

        if (weight <= 0) {
          std::cout << "edge weight in the neighborhood of node " << cur_u + 1 << " cannot be negative: " << weight
                    << std::endl;
        } else if (weight > std::numeric_limits<Weight>::max()) {
          std::cout << "to load this graph, you must increase the width of the Weight datatype due to the weight "
                    << "of an edge in the neighborhood of node " << cur_u + 1 << ": " << weight << std::endl;
        }
      }

      edges.push_back({cur_u, v, weight});
    }
    ++cur_u;
  }

  if (cur_u < n) {
    std::cout << "number of nodes mismatches: the header specifies " << n << " nodes, but there are only " << cur_u
              << " "
              << "nodes in the graph file" << std::endl;
  }
  if (static_cast<idx>(edges.size()) != m) {
    std::cout << "number of edges mismatches: the header specifies " << m << " directed edges, but there are only "
              << edges.size() << " directed edges in the graph file" << std::endl;
  }

  if (total_node_weight > std::numeric_limits<Weight>::max()) {
    std::cout << "to load this graph, you must increase the width of the Weight datatype to represent the total node "
              << "weight: " << total_node_weight << std::endl;
  }
  if (total_edge_weight > std::numeric_limits<Weight>::max()) {
    std::cout << "to load this graph, you must increase the width of the Weight datatype to represent the total edge "
              << "weight: " << total_edge_weight << std::endl;
  }

  std::sort(edges.begin(), edges.end(),
            [](const auto &a, const auto &b) { return a.u < b.u || (a.u == b.u && a.v < b.v); });

  for (std::size_t i = 1; i < edges.size(); ++i) {
    const Edge &prev = edges[i - 1];
    const Edge &cur = edges[i];
    if (prev.u == cur.u && prev.v == cur.v) {
      std::cout << "duplicate edge: " << cur.u << " --> " << cur.v << " with weights " << cur.weight << " and "
                << prev.weight << std::endl;
    }
  }

  std::vector<long> nodes(n + 1);
  for (idx i = 0, j = 0; i < n; ++i) {
    while (j < m && edges[j].u == i) { ++j; }
    nodes[i + 1] = j;
  }

  for (const auto &[u, v, weight] : edges) {
    if (u > v) { continue; }
    const auto end = edges.begin() + nodes[v + 1];
    const auto rev_edge = binary_find(edges.begin() + nodes[v], end, u);

    if (rev_edge == end) { std::cout << "missing reverse edge: of edge " << u + 1 << " --> " << v + 1 << std::endl; }
    if (weight != rev_edge->weight) {
      std::cout << "edge " << u + 1 << " --> " << v + 1 << " has weight " << weight
                << ", but the reverse edge has weight " << rev_edge->weight << std::endl;
    }
  }

  return 0;
}