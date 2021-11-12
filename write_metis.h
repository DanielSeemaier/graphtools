#pragma once

#include "buffered_text_output.h"
#include "definitions.h"
#include "read_edgelist.h"

namespace graphfmt::metis {
void write_format(BufferedTextOutput<> &out, const ID n, const ID m) {
  out.write_int(n).write_char(' ').write_int(m / 2).write_char('\n');
}

void write_format(const std::string &output_filename, const ID n, const ID m) {
  BufferedTextOutput out{tag::create, output_filename};
  write_format(out, n, m);
}

template<typename ProgressLambda>
void write_graph_part(const std::string &filename, const EdgeList &edge_list, const ID from, const ID to,
                      ProgressLambda &&progress_lambda) {
  BufferedTextOutput out(tag::append, filename);

  ID cur_u = from;
  ID edges_written = 0;
  std::size_t iteration_counter = 0;

  for (const auto &[u, v] : edge_list) {
    while (u > cur_u) {
      out.write_char('\n').flush();
      ++cur_u;
    }

    out.write_int(v + 1).write_char(' ').flush();
    ++edges_written;

    if (iteration_counter++ >= 1024 * 1024) {
      progress_lambda(edges_written, edge_list.size());
      iteration_counter = 0;
    }
  }

  // finish last node
  if (!edge_list.empty()) {
    ++cur_u;
    out.write_char('\n');
  }

  // add isolated nodes
  while (to > cur_u) {
    out.write_char('\n').flush();
    ++cur_u;
  }
}

void write_edge(BufferedTextOutput<> &out, const ID last_from, const ID from, const ID to) {
  for (ID cur_u = last_from; cur_u < from; ++cur_u) { out.write_char('\n').flush(); }
  out.write_int(to + 1).write_char(' ').flush();
}

void write_finish(BufferedTextOutput<> &out, const ID last_from, const ID n) {
  for (ID cur_u = last_from; cur_u < n; ++cur_u) { out.write_char('\n').flush(); }
}
} // namespace graphfmt
