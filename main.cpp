#include "definitions.h"
#include "progress.h"
#include "read_edgelist.h"
#include "write_metis.h"

#include <iostream>

using namespace el2metis;

bool file_exists(const std::string &filename) {
  std::ifstream in(filename);
  return in.good();
}

std::string build_output_filename(const std::string &input_filename) {
  const auto dot_pos = input_filename.find_last_of('.');
  if (dot_pos == std::string::npos) { return input_filename + ".graph"; }
  return input_filename.substr(0, dot_pos) + ".graph";
}

int main(const int argc, const char *argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <input-filename>" << std::endl;
    std::exit(1);
  }

  const std::string input_filename = argv[1];
  const std::string output_filename = build_output_filename(input_filename);

  const std::string caption_in = std::string("-> ") + input_filename;
  const std::string caption_out = std::string("<- ") + output_filename;

  auto update_bar = [](auto &progress, const std::size_t col) {
    return [&progress, col](const auto current, const auto max) {
      progress.update(col, 100.0 * current / max);
      progress.render(std::cout);
    };
  };

  auto ignore_progress = [](const auto, const auto) {};

  if (file_exists(input_filename)) {
    MultiColumnProgressBar<2> progress{{40, 40}, {100, 100}, {caption_in, caption_out}};

    ID n, m;
    const auto edge_list = read_edge_list(input_filename, n, m, update_bar(progress, 0));
    progress.finish(0);
    progress.render(std::cout);

    write_header(output_filename, n, m);
    write_graph_part(output_filename, edge_list, 0, n, update_bar(progress, 1));
    progress.finish(1);
    progress.finish();
    progress.render(std::cout);
  } else {
    const std::string first_part = input_filename + "_0";
    if (!file_exists(first_part)) {
      std::cerr << "file does not exist" << std::endl;
      std::exit(1);
    }

    ID global_n, global_m;
    ID to;

    {
      MultiColumnProgressBar<2> progress{{40, 40}, {100, 100}, {caption_in, caption_out}, "0", 3};

      const auto first_edge_list_part = read_edge_list(first_part, global_n, global_m, update_bar(progress, 0));
      progress.finish(0);
      progress.render(std::cout);

      write_header(output_filename, global_n, global_m);
      to = first_edge_list_part.back().first;
      write_graph_part(output_filename, first_edge_list_part, 0, to, update_bar(progress, 1));
      progress.finish(1);
      progress.finish();
      progress.render(std::cout);
    }

    for (std::size_t i = 1;; ++i) {
      MultiColumnProgressBar<2> progress{{40, 40}, {100, 100}, {}, std::to_string(i), 3};

      const ID from = to + 1;

      const std::string part = input_filename + "_" + std::to_string(i);
      if (!file_exists(part)) {
        write_graph_part(output_filename, {}, from, global_n, ignore_progress);
        break;
      }

      ID n, m;
      const auto edge_list_part = read_edge_list(part, n, m, update_bar(progress, 0));
      progress.finish(0);
      progress.render(std::cout);

      to = edge_list_part.back().first;
      write_graph_part(output_filename, edge_list_part, from, to, update_bar(progress, 1));
      progress.finish(1);
      progress.finish();
      progress.render(std::cout);
    }
  }
}
