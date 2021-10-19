#include <algorithm>
#include <cctype>
#include <cerrno>
#include <concepts>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using ID = unsigned long;

namespace mmap_io {
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
} // namespace mmap_io

namespace write_io {
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

char *flush_buf(const int fd, char *buf, char *cur_buf) {
  write(fd, buf, cur_buf - buf);
  return buf;
}
} // namespace write_io

using EdgeList = std::vector<std::pair<ID, ID>>;

bool file_exists(const std::string &filename) {
  std::ifstream in(filename);
  return in.good();
}

EdgeList read_edge_list(const std::string &filename, ID &n, ID &m) {
  using namespace mmap_io;

  EdgeList edge_list;

  auto file = mmap_file_from_disk(filename);
  skip_char(file); // p
  skip_char(file); // ' '
  n = scan_int<ID>(file);
  m = scan_int<ID>(file);
  skip_char(file); // \n

  while (file.current() == 'e') {
    skip_char(file); // e
    skip_char(file); // ' '
    const auto from = scan_int<ID>(file) - 1;
    const auto to = scan_int<ID>(file) - 1;
    skip_char(file); // \n
    edge_list.emplace_back(from, to);
  }

  munmap_file_from_disk(file);

  std::sort(edge_list.begin(), edge_list.end());
  return edge_list;
}

void write_header(const std::string &filename, const ID n, const ID m) {
  using namespace write_io;

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

void write_graph_part(const std::string &filename, const EdgeList &edge_list, const ID from, const ID to) {
  using namespace write_io;
  const int fd = open(filename.c_str(), O_WRONLY | O_APPEND);

  static constexpr std::size_t BUF_SIZE = 16384;
  static constexpr std::size_t BUF_SIZE_LIMIT = BUF_SIZE - 1024;
  char buf[BUF_SIZE];
  char *cur_buf = buf;

  auto flush_if_full = [&]{
    const std::size_t len = cur_buf - buf;
    if (len > BUF_SIZE_LIMIT) {
      cur_buf = flush_buf(fd, buf, cur_buf);
    }
  };

  ID cur_u = from;
  for (const auto &[u, v] : edge_list) {
    while (u > cur_u) {
      cur_buf = write_char(cur_buf, '\n');
      flush_if_full();
      ++cur_u;
    }

    cur_buf = write_int(cur_buf, v + 1);
    cur_buf = write_char(cur_buf, ' ');
    flush_if_full();
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

std::string build_output_filename(const std::string &input_filename) {
  const auto dot_pos = input_filename.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return input_filename + ".graph";
  }
  return input_filename.substr(0, dot_pos) + ".graph";
}

int main(const int argc, const char *argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <input-filename>" << std::endl;
    std::exit(1);
  }

  const std::string input_filename = argv[1];
  const std::string output_filename = build_output_filename(input_filename);

  if (file_exists(input_filename)) {
    ID n, m;
    const auto edge_list = read_edge_list(input_filename, n, m);
    write_header(output_filename, n, m);
    write_graph_part(output_filename, edge_list, 0, n);
  } else {
    const std::string first_part = input_filename + "_0";
    if (!file_exists(first_part)) {
      std::cerr << "file does not exist" << std::endl;
      std::exit(1);
    }

    ID global_n, global_m;
    ID to;

    {
      const auto first_edge_list_part = read_edge_list(first_part, global_n, global_m);
      write_header(output_filename, global_n, global_m);
      to = first_edge_list_part.back().first;
      write_graph_part(output_filename, first_edge_list_part, 0, to);
    }

    for (std::size_t i = 1; ; ++i) {
      const ID from = to + 1;

      const std::string part = input_filename + "_" + std::to_string(i);
      if (!file_exists(part)) {
        write_graph_part(output_filename, {}, from, global_n);
        break;
      }

      ID n, m;
      const auto edge_list_part = read_edge_list(part, n, m);
      to = edge_list_part.back().first;
      write_graph_part(output_filename, edge_list_part, from, to);
    }
  }
}
