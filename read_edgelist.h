#pragma once

#include "definitions.h"

#include <algorithm>
#include <cctype>
#include <execution>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>

namespace el2metis {
namespace internal {
struct MappedFile {
  const int fd;
  std::size_t position;
  const std::size_t length;
  char *contents;

  [[nodiscard]] inline bool valid_position() const { return position < length; }
  [[nodiscard]] inline char current() const { return contents[position]; }
  inline void advance() { ++position; }
};

inline int open_file(const std::string &filename) { return open(filename.c_str(), O_RDONLY); }

inline std::size_t file_size(const int fd) {
  struct stat file_info {};
  fstat(fd, &file_info);
  return static_cast<std::size_t>(file_info.st_size);
}

inline MappedFile mmap_file_from_disk(const std::string &filename) {
  const int fd = open_file(filename);
  const std::size_t length = file_size(fd);

  char *contents = static_cast<char *>(mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0));

  return {
      .fd = fd,
      .position = 0,
      .length = length,
      .contents = contents,
  };
}

inline void munmap_file_from_disk(const MappedFile &mapped_file) {
  munmap(mapped_file.contents, mapped_file.length);
  close(mapped_file.fd);
}

inline void skip_spaces(MappedFile &mapped_file) {
  while (mapped_file.valid_position() && mapped_file.current() == ' ') { mapped_file.advance(); }
}

inline void skip_char(MappedFile &mapped_file) { mapped_file.advance(); }

template<typename Int>
inline Int scan_int(MappedFile &mapped_file) {
  Int number = 0;
  while (mapped_file.valid_position() && std::isdigit(mapped_file.current())) {
    const int digit = mapped_file.current() - '0';
    number = number * 10 + digit;
    mapped_file.advance();
  }
  skip_spaces(mapped_file);
  return number;
}
} // namespace internal

template<typename ProgressLambda>
EdgeList read_edge_list(const std::string &filename, ID &n, ID &m, ProgressLambda &&progress_lambda) {
  using namespace internal;

  EdgeList edge_list;

  auto file = mmap_file_from_disk(filename);
  skip_char(file); // p
  skip_char(file); // ' '
  n = scan_int<ID>(file);
  m = scan_int<ID>(file);
  skip_char(file); // \n

  std::size_t iteration_counter = 0;

  while (file.current() == 'e') {
    skip_char(file); // e
    skip_char(file); // ' '
    const auto from = scan_int<ID>(file) - 1;
    const auto to = scan_int<ID>(file) - 1;
    skip_char(file); // \n
    edge_list.emplace_back(from, to);

    if (iteration_counter++ >= 1024 * 1024) {
      progress_lambda(file.position, file.length);
      iteration_counter = 0;
    }
  }

  munmap_file_from_disk(file);

  std::sort(std::execution::par, edge_list.begin(), edge_list.end());
  return edge_list;
}
} // namespace el2metis