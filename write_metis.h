#pragma once

#include "definitions.h"
#include "read_edgelist.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace el2metis {
namespace internal {
char *write_char(char *buffer, const char value) {
  *buffer = value;
  return buffer + 1;
}

template<typename Int>
char *write_int(char *buffer, Int value) {
  static char rev_buffer[80];

  int pos = 0;
  do {
    rev_buffer[pos++] = value % 10;
    value /= 10;
  } while (value > 0);

  while (pos > 0) { *(buffer++) = '0' + rev_buffer[--pos]; }

  return buffer;
}

char *flush_buf(const int fd, char *buf, const char *cur_buf) {
  write(fd, buf, cur_buf - buf);
  return buf;
}
} // namespace internal

void write_header(const std::string &filename, const ID n, const ID m) {
  using namespace internal;

  const int fd = open(filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  char buf[1024];
  char *cur_buf = buf;

  cur_buf = write_int(cur_buf, n);
  cur_buf = write_char(cur_buf, ' ');
  cur_buf = write_int(cur_buf, m / 2);
  cur_buf = write_char(cur_buf, '\n');
  flush_buf(fd, buf, cur_buf);
  close(fd);
}

template<typename ProgressLambda>
void write_graph_part(const std::string &filename, const EdgeList &edge_list, const ID from, const ID to,
                      ProgressLambda &&progress_lambda) {
  using namespace internal;
  const int fd = open(filename.c_str(), O_WRONLY | O_APPEND);

  static constexpr std::size_t BUF_SIZE = 16384;
  static constexpr std::size_t BUF_SIZE_LIMIT = BUF_SIZE - 1024;
  char buf[BUF_SIZE];
  char *cur_buf = buf;

  auto flush_if_full = [&] {
    const std::size_t len = cur_buf - buf;
    if (len > BUF_SIZE_LIMIT) { cur_buf = flush_buf(fd, buf, cur_buf); }
  };

  ID cur_u = from;

  ID edges_written = 0;
  std::size_t iteration_counter = 0;

  for (const auto &[u, v] : edge_list) {
    while (u > cur_u) {
      cur_buf = write_char(cur_buf, '\n');
      flush_if_full();
      ++cur_u;
    }

    cur_buf = write_int(cur_buf, v + 1);
    cur_buf = write_char(cur_buf, ' ');
    flush_if_full();
    ++edges_written;

    if (iteration_counter++ >= 1024 * 1024) {
      progress_lambda(edges_written, edge_list.size());
      iteration_counter = 0;
    }
  }

  // finish last node
  if (!edge_list.empty()) {
    ++cur_u;
    cur_buf = write_char(cur_buf, '\n');
  }

  // add isolated nodes
  while (to > cur_u) {
    cur_buf = write_char(cur_buf, '\n');
    flush_if_full();
    ++cur_u;
  }

  flush_buf(fd, buf, cur_buf);
  close(fd);
}
} // namespace el2metis
