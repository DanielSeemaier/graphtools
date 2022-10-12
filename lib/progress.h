#pragma once

#include <array>
#include <iomanip>
#include <iostream>
#include <string>

namespace graphtools {
template <std::size_t kNumColumns = 1>
class MultiColumnProgressBar {
    static constexpr char             kLeftBorderSym  = '[';
    static constexpr char             kRightBorderSym = ']';
    static constexpr char             kProgressBarSym = '=';
    static constexpr char             kPrefixDelSym   = ' ';
    static constexpr std::string_view kSpinner        = "|/-\\";

public:
    MultiColumnProgressBar(
        const std::array<std::size_t, kNumColumns>& widths, const std::array<std::size_t, kNumColumns>& max_values,
        const std::array<std::string, kNumColumns>& captions = {}, std::string prefix = "",
        const std::size_t prefix_width = 0
    )
        : _widths{widths},
          _values{},
          _max_values{max_values},
          _captions{captions},
          _prefix{std::move(prefix)},
          _prefix_width{prefix_width} {}

    void update(const std::size_t column, std::size_t value) {
        _values[column]        = value;
        _spinner_index[column] = (_spinner_index[column] + 1) % kSpinner.length();
    }

    void finish(const std::size_t column) {
        _values[column] = _max_values[column];
    }

    void finish() {
        _final_render = true;
    }

    void render(std::ostream& out) {
        if (_initial_render && !_captions.empty()) {
            render_captions(out);
            _initial_render = false;
        } else if (!_initial_render) {
            std::cout << "\r" << std::flush;
        }
        render_progress_bars(out);
        if (_final_render) {
            std::cout << std::endl;
        }
    }

private:
    void render_captions(std::ostream& out) {
        if (has_captions()) {
            if (!_prefix.empty()) {
                out << std::string(_prefix_width + 1, ' ');
            }
            for (std::size_t i = 0; i < kNumColumns; ++i) {
                out << std::setfill(' ') << std::setw(_widths[i] - 1) << _captions[i] << " ";
            }
            out << std::endl;
        }
    }

    void render_progress_bars(std::ostream& out) {
        if (!_prefix.empty()) {
            if (_prefix_width != 0) {
                out << std::setfill(' ') << std::setw(_prefix_width);
            }
            out << _prefix << kPrefixDelSym << std::flush;
        }

        for (std::size_t i = 0; i < kNumColumns; ++i) {
            const std::size_t progress_width = _widths[i] - 2;
            const double      progress       = 1.0 * _values[i] / _max_values[i];

            const int         percentage       = static_cast<int>(progress * 100.0);
            const std::string percentage_str   = std::to_string(percentage) + "%";
            const std::size_t percentage_width = percentage_str.length();
            const std::size_t bar_max_width    = progress_width - percentage_width;
            const std::size_t bar_full_width   = progress_width * progress;
            const std::size_t bar_width        = std::min<std::size_t>(bar_full_width, bar_max_width);

            out << kLeftBorderSym;
            if (bar_width > 0) {
                out << std::string(bar_width - 1, kProgressBarSym);
                if (bar_full_width <= bar_max_width) {
                    out << spinner_sym(i);
                } else {
                    out << kProgressBarSym;
                }
            }
            out << std::string(bar_max_width - bar_width, ' ') << percentage_str << kRightBorderSym;
        }
    }

    [[nodiscard]] bool has_captions() const {
        for (std::size_t i = 0; i < kNumColumns; ++i) {
            if (!_captions[i].empty()) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] char spinner_sym(const std::size_t column) const {
        return kSpinner[_spinner_index[column]];
    }

    std::array<std::size_t, kNumColumns> _widths;
    std::array<std::size_t, kNumColumns> _values{};
    std::array<std::size_t, kNumColumns> _max_values;
    std::array<std::string, kNumColumns> _captions;
    std::array<std::size_t, kNumColumns> _spinner_index{};
    std::string                          _prefix;
    std::size_t                          _prefix_width;
    bool                                 _initial_render{true};
    bool                                 _final_render{false};
};
} // namespace graphtools

