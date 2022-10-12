#pragma once

#include <iostream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "definitions.h"

namespace graphtools {
struct create_tag {};
struct append_tag {};

namespace tag {
constexpr create_tag create;
constexpr append_tag append;
} // namespace tag

template <std::size_t kBufferSize = 1024 * 1024, std::size_t kBufferSizeLimit = kBufferSize - 1024>
class BufferedTextOutput {
public:
    BufferedTextOutput(create_tag, const std::string& filename)
        : _fd{_fd = open(filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)} {
        if (_fd < 0) {
            std::cout << "cannot write to " << filename << std::endl;
            std::exit(0);
        }
    }

    BufferedTextOutput(append_tag, const std::string& filename) : _fd{open(filename.c_str(), O_WRONLY | O_APPEND)} {
        if (_fd < 0) {
            std::cout << "cannot append to " << filename << std::endl;
            std::exit(0);
        }
    }

    ~BufferedTextOutput() {
        force_flush();
        close(_fd);
    }

    BufferedTextOutput& write_char(const char ch) {
        *(_buffer_pos)++ = ch;
        return *this;
    }

    template <typename Int>
    BufferedTextOutput& write_int(Int value) {
        static char rev_buffer[80];

        int pos = 0;
        do {
            rev_buffer[pos++] = value % 10;
            value /= 10;
        } while (value > 0);

        while (pos > 0) {
            *(_buffer_pos++) = '0' + rev_buffer[--pos];
        }
        return *this;
    }

    BufferedTextOutput& flush() {
        if (_buffer_pos - _buffer >= kBufferSizeLimit) {
            force_flush();
        }
        return *this;
    }

private:
    void force_flush() {
        write(_fd, _buffer, _buffer_pos - _buffer);
        _buffer_pos = _buffer;
    }

    int   _fd;
    char  _buffer[kBufferSize]{0};
    char* _buffer_pos{_buffer};
};
} // namespace graphtools

