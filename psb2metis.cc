#include <fstream>
#include <string>

#include "lib/buffered_text_output.h"
#include "lib/definitions.h"
#include "lib/progress.h"
#include "lib/write_metis.h"

#include "CLI11.hpp"

using namespace graphtools;

template <typename T, std::size_t length>
T read(std::ifstream& in) {
    if constexpr (std::is_same_v<T, std::string>) {
        std::string str(length, ' ');
        in.read(str.data(), length);
        return str;
    } else {
        std::array<std::uint8_t, length> data;
        in.read(reinterpret_cast<char*>(data.data()), length);
        T var = 0;
        for (std::size_t i = 0; i < length; ++i) {
            var <<= 8;
            var |= data[i];
        }
        return var;
    }
}

struct RGB {
    std::uint64_t             width;
    std::uint64_t             height;
    std::vector<std::uint8_t> R;
    std::vector<std::uint8_t> G;
    std::vector<std::uint8_t> B;
};

RGB parse_simple_psb(const std::string& filename) {
    std::ifstream in(filename);

    in.seekg(0, std::ios_base::end);
    auto size = in.tellg();
    in.seekg(0);
    std::cout << "File size: " << size << std::endl;

    auto signature    = read<std::string, 4>(in);
    auto version      = read<std::uint16_t, 2>(in);
    auto reserved     = read<std::uint64_t, 6>(in);
    auto num_channels = read<std::uint16_t, 2>(in);
    auto height       = read<std::uint32_t, 4>(in);
    auto width        = read<std::uint32_t, 4>(in);
    auto depth        = read<std::uint16_t, 2>(in);
    auto color_mode   = read<std::uint16_t, 2>(in);
    std::cout << "Signature: " << signature << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "Reserved: " << reserved << std::endl;
    std::cout << "# channels: " << num_channels << std::endl;
    std::cout << "Height: " << height << std::endl;
    std::cout << "Width: " << width << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    std::cout << "Color mode: " << color_mode << std::endl;

    if (version != 2) {
        std::cerr << "Error: unexpected file format " << version
                  << " (only supporting 2 = PSB; add support for 1 = PSD when needed)\n";
        std::exit(1);
    }
    if (depth != 8) {
        std::cerr << "Error: unexpected channel depth " << depth << " (only supporting 8)\n";
        std::exit(1);
    }
    if (num_channels != 3) {
        std::cerr << "Error: unexpected number of channels " << num_channels << " (only supporting 3)\n";
        std::exit(1);
    }
    if (color_mode != 3) {
        std::cerr << "Error: unexpected color mode " << color_mode << " (only supporting 3 = RGB)\n";
        std::exit(1);
    }

    auto color_section_length = read<std::uint32_t, 4>(in);
    std::cout << "Color Mode Data Length: " << color_section_length << " (ignoring)" << std::endl;
    in.seekg(color_section_length, std::ios_base::cur);

    auto image_resources_length = read<std::uint32_t, 4>(in);
    std::cout << "Image Resources Length: " << image_resources_length << " (ignoring)" << std::endl;
    in.seekg(image_resources_length, std::ios_base::cur);

    auto layer_mask_length = read<std::uint64_t, 8>(in); // PSD = 4, PSB = 8
    std::cout << "Layer and Mask Information Length: " << layer_mask_length << " (ignoring)" << std::endl;
    in.seekg(layer_mask_length, std::ios_base::cur);

    auto compression = read<std::uint16_t, 2>(in);
    std::cout << "Image Data Compression: " << compression << std::endl;
    std::cout << " -- current position in file: " << in.tellg() << std::endl;
    std::cout << " -- remaining bytes: " << size - in.tellg() << std::endl;
    std::cout << " -- div. by # channels: " << ((size - in.tellg()) % num_channels == 0 ? "yes" : "no") << std::endl;
    std::cout << " -- entries per channel: " << (size - in.tellg()) / num_channels << std::endl;
    std::cout << " --  ... expected: " << static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height)
              << std::endl;

    const std::uint64_t       num_pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    std::vector<std::uint8_t> red(num_pixels);
    std::vector<std::uint8_t> green(num_pixels);
    std::vector<std::uint8_t> blue(num_pixels);
    in.read(reinterpret_cast<char*>(red.data()), num_pixels);
    in.read(reinterpret_cast<char*>(green.data()), num_pixels);
    in.read(reinterpret_cast<char*>(blue.data()), num_pixels);
    std::cout << "Finished reading, position: " << in.tellg() << std::endl;

    return {width, height, std::move(red), std::move(green), std::move(blue)};
}

int main(int argc, char* argv[]) {
    std::string  input_filename;
    std::string  output_filename;
    std::string  tga_output_filename;
    std::int16_t tga_size = 255;
    // std::string  weight_model      = "l2";
    bool periodic_boundary = false;

    CLI::App app("trimmetis");
    app.add_option("input psb", input_filename, "Input PSB file")->check(CLI::ExistingFile)->required();
    app.add_option(
        "--tga", tga_output_filename,
        "If set, output the top left corner of the image to the specified TGA file (as a sanity check)"
    );
    app.add_option("--tga-size", tga_size, "Size of the TGA output in number of rows / cols");
    app.add_option("-o,--output", output_filename, "Output graph");
    // app.add_option("--weight-model", weight_model, "Edge weights, possible options are: {l2}")
    //     ->check(CLI::IsMember({"l2"}));
    app.add_flag("--periodic-boundary", periodic_boundary);
    CLI11_PARSE(app, argc, argv);

    RGB               rgb    = parse_simple_psb(input_filename);
    const std::size_t width  = rgb.width;
    const std::size_t height = rgb.height;
    const auto&       R      = rgb.R;
    const auto&       G      = rgb.G;
    const auto&       B      = rgb.B;

    if (!output_filename.empty()) {
        const ID n = width * height;
        const ID m = periodic_boundary ? (2 * 4 * n) : (2 * 4 * n - 2 * width - 2 * height);
        metis::write_format(output_filename, n, m, false, true);

        BufferedTextOutput<> out(tag::append, output_filename);
        auto write_edge = [&](const std::size_t y1, const std::size_t x1, const std::size_t y2, const std::size_t x2) {
            const ID v = y2 * width + x2;

            const std::size_t  pos1      = y1 * width + x1;
            const std::size_t  pos2      = y2 * width + x2;
            const std::uint8_t Rd        = std::max(R[pos1], R[pos2]) - std::min(R[pos1], R[pos2]);
            const std::uint8_t Gd        = std::max(G[pos1], G[pos2]) - std::min(G[pos1], G[pos2]);
            const std::uint8_t Bd        = std::max(B[pos1], B[pos2]) - std::min(B[pos1], B[pos2]);
            const ID           l2_weight = std::sqrt(Rd * Rd + Gd * Gd + Bd * Bd);

            out.write_int(v + 1).write_char(' ').write_int(l2_weight).write_char(' ').flush();
        };
        auto next_node = [&] {
            out.write_char('\n').flush();
        };

        MultiColumnProgressBar<> progress({80}, {height}, {"Generating Graph"});

        // 0,0
        if (periodic_boundary) {
            write_edge(0, 0, height - 1, 0); // top
        }
        write_edge(0, 0, 0, 1); // right
        write_edge(0, 0, 1, 0); // bottom
        if (periodic_boundary) {
            write_edge(0, 0, 0, width - 1); // left
        }
        next_node();
        // 0,x
        if (periodic_boundary) {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                write_edge(0, x, height - 1, x);
                write_edge(0, x, 0, x + 1);
                write_edge(0, x, 1, x);
                write_edge(0, x, 0, x - 1);
                next_node();
            }
        } else {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                write_edge(0, x, 0, x + 1);
                write_edge(0, x, 1, x);
                write_edge(0, x, 0, x - 1);
                next_node();
            }
        }
        // 0,width-1
        if (periodic_boundary) {
            write_edge(0, width - 1, height - 1, width - 1);
        }
        write_edge(0, width - 1, 0, 0);
        if (periodic_boundary) {
            write_edge(0, width - 1, 1, width - 1);
        }
        write_edge(0, width - 1, 0, 0);
        next_node();

        for (std::size_t y = 1; y + 1 < height; ++y) {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                write_edge(y, x, y - 1, x);
                write_edge(y, x, y, x + 1);
                write_edge(y, x, y + 1, x);
                write_edge(y, x, y, x - 1);
                next_node();
            }
            progress.update(0, y);
            progress.render(std::cout);
        }

        write_edge(height - 1, 0, height - 2, 0);
        write_edge(height - 1, 0, height - 1, 1);
        if (periodic_boundary) {
            write_edge(height - 1, 0, 0, 0);
            write_edge(height - 1, 0, height - 1, width - 1);
        }
        next_node();
        if (periodic_boundary) {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                write_edge(height - 1, x, height - 2, x);
                write_edge(height - 1, x, height - 1, x + 1);
                write_edge(height - 1, x, 0, x);
                write_edge(height - 1, x, height - 1, x - 1);
                next_node();
            }
        } else {
            for (std::size_t x = 1; x + 1 < width; ++x) {
                write_edge(height - 1, x, height - 2, x);
                write_edge(height - 1, x, height - 1, x + 1);
                write_edge(height - 1, x, 0, x);
                write_edge(height - 1, x, height - 1, x - 1);
                next_node();
            }
        }
        write_edge(height - 1, width - 1, height - 2, width - 1);
        if (periodic_boundary) {
            write_edge(height - 1, width - 1, height - 1, 0);
            write_edge(height - 1, width - 1, 0, width - 1);
        }
        write_edge(height - 1, width - 1, height - 1, width - 2);
        next_node();

        progress.finish();
        progress.render(std::cout);
    }

    if (!tga_output_filename.empty()) {
        std::array<std::uint8_t, 18> tga;
        tga[0] = 0;                                     // No image ID field
        tga[1] = 0;                                     // No color map included
        tga[2] = 2;                                     // Uncompressed true-color image
        tga[3] = tga[4] = tga[5] = tga[6] = tga[7] = 0; // No color map
        tga[8] = tga[9] = tga[10] = tga[11] = 0;        // Start at 0,0
        tga[12] = tga[14] = tga_size & 255;             // Width
        tga[13] = tga[15] = tga_size >> 8;
        tga[16]           = 24; // 3 * 8 bits per pixel
        tga[17]           = 32; // Top-to-bottom, left-to-right

        std::vector<std::uint8_t> pixels(tga_size * tga_size * 3);
        std::size_t               p = 0;
        for (std::size_t y = 0; y < tga_size; ++y) {
            for (std::size_t x = 0; x < tga_size; ++x) {
                const std::size_t op = y * width + x;
                pixels[p++]          = B[op];
                pixels[p++]          = G[op];
                pixels[p++]          = R[op];
            }
        }

        std::ofstream out("demo.tga");
        out.write(reinterpret_cast<char*>(tga.data()), tga.size());
        out.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        out.close();
    }
}
