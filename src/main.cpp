// MIT License
#include "msxdisa/analyzer.hpp"
#include "msxdisa/zma_writer.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string input_path;
    std::string output_path;
    std::optional<std::uint16_t> origin;
    std::optional<std::uint16_t> start;
    msxdisa::MapperType mapper = msxdisa::MapperType::None;
};

std::uint16_t parse_address(const std::string& text, const std::string& option)
{
    std::size_t consumed = 0;
    unsigned long value = 0;
    try {
        value = std::stoul(text, &consumed, 0);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid address for " + option + ": " + text);
    }
    if (consumed != text.size() || value > 0xFFFF) {
        throw std::runtime_error("address out of range for " + option + ": " + text);
    }
    return static_cast<std::uint16_t>(value);
}

msxdisa::MapperType parse_mapper(const std::string& name)
{
    if (name == "none") return msxdisa::MapperType::None;
    if (name == "ascii16") return msxdisa::MapperType::ASCII16;
    if (name == "ascii8") return msxdisa::MapperType::ASCII8;
    if (name == "konami") return msxdisa::MapperType::Konami;
    if (name == "scc") return msxdisa::MapperType::SCC;
    throw std::runtime_error("unknown mapper: " + name);
}

void print_usage(std::ostream& output)
{
    output << "Usage: msxdisa [options] <rom-file>\n"
              "Options:\n"
              "  -o, --output <file>  Write assembly to file (default: stdout)\n"
              "      --origin <addr>  Override the ROM load address\n"
              "      --start <addr>   Override the code entry address\n"
              "      --mapper <type>  Select none, ascii16, ascii8, konami, or scc\n"
              "  -h, --help           Show this help\n";
}

Options parse_options(int argc, char** argv)
{
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto next_value = [&](const std::string& option) -> std::string {
            if (index + 1 >= argc) throw std::runtime_error("missing value for " + option);
            return argv[++index];
        };

        if (argument == "-h" || argument == "--help") {
            print_usage(std::cout);
            std::exit(0);
        } else if (argument == "-o" || argument == "--output") {
            options.output_path = next_value(argument);
        } else if (argument == "--origin") {
            options.origin = parse_address(next_value(argument), argument);
        } else if (argument == "--start") {
            options.start = parse_address(next_value(argument), argument);
        } else if (argument == "--mapper") {
            options.mapper = parse_mapper(next_value(argument));
        } else if (!argument.empty() && argument[0] == '-') {
            throw std::runtime_error("unknown option: " + argument);
        } else if (options.input_path.empty()) {
            options.input_path = argument;
        } else {
            throw std::runtime_error("unexpected argument: " + argument);
        }
    }
    if (options.input_path.empty()) throw std::runtime_error("missing ROM file");
    return options;
}

std::vector<std::uint8_t> read_file(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input file: " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void disassemble(std::ostream& output, const std::vector<std::uint8_t>& rom, const Options& options)
{
    const auto analysis = msxdisa::analyze_rom(rom, options.mapper, options.origin, options.start);
    msxdisa::write_zma(output, analysis);
}

}

int main(int argc, char** argv)
{
    try {
        const auto options = parse_options(argc, argv);
        const auto rom = read_file(options.input_path);
        if (options.output_path.empty()) {
            disassemble(std::cout, rom, options);
        } else {
            std::ofstream output(options.output_path, std::ios::binary);
            if (!output) throw std::runtime_error("cannot open output file: " + options.output_path);
            disassemble(output, rom, options);
            if (!output) throw std::runtime_error("failed while writing output file: " + options.output_path);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "msxdisa: " << error.what() << "\n";
        std::cerr << "Try 'msxdisa --help' for usage.\n";
        return 1;
    }
}