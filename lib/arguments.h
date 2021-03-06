#pragma once

#include "definitions.h"

#include <cctype>
#include <concepts>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace graphtools {
/**
 * Simple command line arguments parser that requires minimal boilerplate code, built upon getopt.
 */
class Arguments {
public:
  // Preset of parsing functions to be used as `Transformer` argument in Group::argument()
  template<typename Type>
  static Type parse_number(const char *arg) {
    if constexpr (std::is_floating_point_v<Type>) {
      return static_cast<Type>(std::strtod(arg, nullptr));
    } else {
      static_assert(std::is_convertible_v<Type, long>);
      return static_cast<Type>(std::stol(arg, nullptr, 10));
    }
  }

  static std::string parse_string(const char *arg) { return arg; }

  static bool parse_bool(const char *_arg) {
    if (_arg == nullptr) { return true; }
    const std::string arg = _arg;
    return arg == "1" || arg == "yes" || arg == "on" || arg == "true" || arg.empty();
  }

  using Setter = std::function<void(const char *)>;

  struct Argument {
    char short_name{};
    std::string long_name{};
    int argument_type{0};
    Setter lambda{};
    std::string description{};
    std::string default_description{};
    bool vararg{false};
    std::vector<std::string> extended_description{};
  };

  /**
   * A group is a logical container for individual arguments. Has no effect on argument parsing, but helps structuring
   * the output of `--help`.
   * Use `Arguments::group()` to create groups, then use `Group::argument()` to add arguments to the group.
   */
  struct Group {
    Group(std::string name, Arguments *parent, const bool mandatory, std::string code)
        : name(std::move(name)),
          parent(parent),
          mandatory(mandatory),
          code(std::move(code)) {}

    //! Argument whose value can be parsed using `std::strtol()`.
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
    Group &argument(const std::string &lname, const std::string &description, T *storage, const char sname = 0) {
      return argument(lname, description, storage, Arguments::parse_number<T>(), sname);
    }

    //! Argument whose value is a string.
    Group &argument(const std::string &lname, const std::string &description, std::string *storage,
                    const char sname = 0) {
      return argument(lname, description, storage, Arguments::parse_string, sname);
    }

    //! Argument whose value is a bool. `1`, `true`, `on`, `yes` and <empty> are interpreted as `true`, all other values
    //! as `false`.
    Group &argument(const std::string &lname, const std::string &description, bool *storage, const char sname = 0) {
      argument(lname, description, storage, Arguments::parse_bool, sname, optional_argument);
      return *this;
    }

    /**
     * Adds an argument with custom data type. Must supply an invocable object that transforms the value passed to the
     * argument to the custom data type.
     *
     * @tparam Type Custom data type.
     * @tparam Transformer Type of an invocable object that transforms a string to `Type`.
     * @param lname Long name for this argument, must be present.
     * @param description Description shown in `--help` next to the argument.
     * @param storage Address to where we store the argument passed to this argument.
     * @param transformer Invocable object to parse the textual value passed to this argument.
     * @param sname Optional short name for this argument.
     * @return `this`
     */
    template<typename Type, typename Transformer>
    Group &argument(const std::string &lname, const std::string &description, Type *storage, Transformer &&transformer,
                    char sname = 0, int argument_type = required_argument) {
      static_assert(std::is_convertible_v<std::result_of_t<Transformer(const char *)>, Type>,
                    "Result type of transforming function must be convertible to the data type of the storage "
                    "pointer.");

      auto setter = [storage, transformer](const char *arg) { *storage = transformer(arg); };
      std::stringstream default_arg;
      if (!mandatory) { default_arg << *storage; }
      return argument(lname, argument_type, std::move(setter), description, default_arg.str(), sname);
    }

    //! Argument without storage address, instead we call a lambda when encountering the argument.
    Group &argument(const std::string &lname, const int argument_type, Setter lambda, const std::string &description,
                    const std::string &default_description, const char sname = 0) {
      parent->arguments.push_back({
          .short_name = sname,                       //
          .long_name = lname,                        //
          .argument_type = argument_type,            //
          .lambda = std::move(lambda),               //
          .description = description,                //
          .default_description = default_description //
      });
      arguments.push_back(parent->arguments.back());
      return *this;
    }

    Group &line(std::string text) {
      if (arguments.empty()) {
        extended_description.push_back(std::move(text));
      } else {
        arguments.back().extended_description.push_back(std::move(text));
      }
      return *this;
    }

    std::vector<Argument> arguments{};
    std::string name{};
    Arguments *parent{};
    bool mandatory{};
    std::string code{};
    std::vector<std::string> extended_description;
  };

  Group &group(const std::string &name, const std::string &code = "", bool mandatory = false) {
    groups.emplace_back(name, this, mandatory, code);
    return groups.back();
  }

  Group &positional() { return positional_group; }

  void parse(int argc, char *argv[], const bool enforce_positional_arguments = true) {
    auto options = create_options();
    options.push_back({"help", optional_argument, nullptr, 0});
    std::swap(options.back(), options[options.size() - 2]); // move 0 entry to the back

    auto options_string = create_options_string();
    int c, index = 0;
    while ((c = getopt_long(argc, argv, options_string.c_str(), options.data(), &index)) != -1) {
      if (c == 0 && std::string(options[index].name) == "help") { // catch --help
        print_help(argc, argv, optarg);
        std::exit(0);
      }

      auto arg = (c == 0) ? find_by_long_name(options[index].name) : find_by_short_name(static_cast<char>(c));
      if (arg == arguments.end()) {
        std::cout << "bad argument (see above)" << std::endl;
        std::exit(1);
      }
      arg->lambda(optarg);
    }

    if (!positional_group.arguments.empty()) {
      const auto actual_num_pos_args = static_cast<std::size_t>(argc - optind);
      const std::size_t expected_pos_args = positional_group.arguments.size();
      const bool has_vararg = std::any_of(positional_group.arguments.begin(), positional_group.arguments.end(),
                                          [](const auto &arg) { return arg.vararg; });
      //if (enforce_positional_arguments && !has_vararg && actual_num_pos_args != expected_pos_args) {
      //  std::cout << "unexpected number of positional arguments: got " << actual_num_pos_args << ", expected "
      //            << expected_pos_args << std::endl;
      //  std::exit(1);
      //}

      for (int j = 0, i = optind; i < argc; ++i) {
        positional_group.arguments[j].lambda(argv[i]);
        if (!positional_group.arguments[j].vararg) { ++j; }
      }
    }
  }

  void print_help(int, char *argv[], const char *section) {
    using namespace std::string_literals;
    const bool full = section == nullptr;

    // print Usage: ...
    if (full) {
      std::cout << "Usage: " << argv[0] << " ";
      if (!groups.empty()) { std::cout << "options... "; }
      for (const auto &positional_argument : positional_group.arguments) {
        std::cout << "<" << positional_argument.long_name;
        if (positional_argument.vararg) { std::cout << "..."; }
        std::cout << "> ";
      }
      std::cout << "\n\n";
    }

    // print description of positional arguments
    if (full && !positional_group.arguments.empty()) {
      std::cout << "Positional arguments are:\n";
      const std::size_t padding_length = compute_group_padding(positional_group);
      for (const auto &positional_argument : positional_group.arguments) {
        const std::string prefix = "<"s + positional_argument.long_name + ">"s;
        const std::string padding(padding_length - prefix.size(), '.');
        std::cout << " " << prefix << " ." << padding << ". " << positional_argument.description << "\n";
        for (const auto &line : positional_argument.extended_description) {
          std::cout << std::string(padding_length + 5, ' ') << line << "\n";
        }
      }
      std::cout << "\n";
    }

    // print description of mandatory and optional options
    bool printed_group{false};
    for (const auto &group : groups) {
      if (!full && group.code != optarg) { continue; }
      printed_group = true;

      std::cout << group.name << ":\n";

      const std::size_t padding_length = compute_group_padding(group);
      for (const auto &argument : group.arguments) {
        const std::string prefix = create_description_prefix(argument);
        const std::string padding(padding_length - prefix.size(), '.');
        std::cout << " " << prefix << " ." << padding << ". " << argument.description << "\n";
        for (const auto &line : argument.extended_description) {
          std::cout << "   " << std::string(padding_length, ' ') << "  " << line << "\n";
        }
        if (!argument.default_description.empty()) {
          std::cout << "   " << std::string(padding_length, ' ') << "  Default: <arg>=" << argument.default_description
                    << "\n";
        }
      }
      std::cout << "\n";
    }

    if (!full && !printed_group) {
      std::cout << "No group with code " << optarg << "; run with --help to see all options\n";
      std::exit(1);
    }
  }

private:
  friend Group;

  static std::size_t compute_group_padding(const Group &group) {
    std::size_t max_padding = 0;
    for (const auto &arg : group.arguments) {
      max_padding = std::max(max_padding, create_description_prefix(arg).size());
    }
    return max_padding;
  }

  static std::string create_description_prefix(const Argument &argument) {
    std::stringstream result;
    if (argument.short_name != 0) { result << "-" << argument.short_name << ", "; }
    result << "--" << argument.long_name;
    if (argument.argument_type == required_argument) {
      result << "=<arg>";
    } else if (argument.argument_type == optional_argument) {
      result << "[=<arg>]";
    }
    return result.str();
  }

  [[nodiscard]] std::vector<Argument>::const_iterator find_by_short_name(const char short_name) const {
    return std::find_if(arguments.begin(), arguments.end(),
                        [&short_name](const Argument &arg) { return arg.short_name == short_name; });
  }

  [[nodiscard]] std::vector<Argument>::const_iterator find_by_long_name(const std::string &long_name) const {
    return std::find_if(arguments.begin(), arguments.end(),
                        [&long_name](const Argument &arg) { return arg.long_name == long_name; });
  }

  [[nodiscard]] std::vector<struct option> create_options() const {
    std::vector<struct option> options;
    for (const auto &argument : arguments) {
      options.push_back({argument.long_name.c_str(), argument.argument_type, nullptr, argument.short_name});
    }
    options.push_back({nullptr, 0, nullptr, 0});
    return options;
  }

  [[nodiscard]] std::string create_options_string() const {
    std::string options;
    for (const auto &argument : arguments) {
      const char c = argument.short_name;
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        options += c;
        if (argument.argument_type == required_argument) { options += ':'; }
      }
    }
    return options;
  }

  std::vector<Argument> arguments{};
  std::vector<Group> groups{};
  Group positional_group{"__positional__", this, false, ""};
};
} // namespace kaminpar