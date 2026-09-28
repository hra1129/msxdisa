// MIT License
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace msxdisa {

enum class MapperType {
    None,
    ASCII16,
    ASCII8,
    Konami,
    SCC
};

enum class ByteKind {
    Unexamined,
    ByteData,
    WordLow,
    WordHigh,
    Instruction
};

struct ByteState {
    ByteKind kind = ByteKind::Unexamined;
    bool has_label = false;
    std::uint8_t instruction_length = 0;
    std::optional<std::size_t> target_bank;
};

struct AnalyzedBank {
    std::size_t number = 0;
    std::size_t file_offset = 0;
    std::uint16_t cpu_origin = 0;
    std::vector<std::uint8_t> bytes;
    std::vector<ByteState> states;
};

struct RomAnalysis {
    bool has_header = false;
    MapperType mapper = MapperType::None;
    std::uint16_t origin = 0;
    std::uint16_t entry = 0;
    std::vector<AnalyzedBank> banks;
};

RomAnalysis analyze_rom(const std::vector<std::uint8_t>& rom, MapperType mapper,
                        std::optional<std::uint16_t> origin_override,
                        std::optional<std::uint16_t> entry_override);

}