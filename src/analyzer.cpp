// MIT License
#include "msxdisa/analyzer.hpp"

#include "msxdisa/z80_decoder.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <stdexcept>
#include <utility>

namespace msxdisa {
namespace {

struct WorkItem {
    std::size_t bank_index;
    std::size_t offset;
};

std::uint16_t read_word(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset] |
        (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
}

std::size_t bank_size_for(MapperType mapper, std::size_t rom_size)
{
    switch (mapper) {
    case MapperType::ASCII16:
        return 0x4000;
    case MapperType::ASCII8:
    case MapperType::Konami:
    case MapperType::SCC:
        return 0x2000;
    case MapperType::None:
        return rom_size;
    }
    return rom_size;
}

std::uint16_t bank_origin(std::size_t bank_number, MapperType mapper, std::uint16_t first_origin)
{
    if (mapper == MapperType::None) return first_origin;

    const std::size_t page_size = mapper == MapperType::ASCII16 ? 0x4000 : 0x2000;
    const std::size_t page_count = 0x8000 / page_size;
    const std::size_t first_page = (first_origin - 0x4000) / page_size;
    const std::size_t page = (first_page + bank_number) % page_count;
    return static_cast<std::uint16_t>(0x4000 + page * page_size);
}

std::optional<std::uint16_t> absolute_operand(const std::vector<std::uint8_t>& bytes,
                                              std::size_t offset, std::size_t length)
{
    const auto opcode = bytes[offset];
    if (length == 3) {
        if ((opcode & 0xCF) == 0x01 || opcode == 0x22 || opcode == 0x2A ||
            opcode == 0x32 || opcode == 0x3A || opcode == 0xC3 || opcode == 0xCD ||
            (opcode & 0xC7) == 0xC2 || (opcode & 0xC7) == 0xC4) {
            return read_word(bytes, offset + 1);
        }
    } else if (length == 4 && offset + 3 < bytes.size()) {
        if (opcode == 0xED && (bytes[offset + 1] & 0xC7) == 0x43) {
            return read_word(bytes, offset + 2);
        }
        if ((opcode == 0xDD || opcode == 0xFD) &&
            (bytes[offset + 1] == 0x21 || bytes[offset + 1] == 0x22 ||
             bytes[offset + 1] == 0x2A)) {
            return read_word(bytes, offset + 2);
        }
    }
    return {};
}

std::uint16_t infer_bank_origin(const AnalyzedBank& bank, MapperType mapper)
{
    const std::size_t page_size = mapper == MapperType::ASCII16 ? 0x4000 : 0x2000;
    const std::size_t page_count = 0x8000 / page_size;
    std::array<std::size_t, 4> references{};
    std::size_t offset = bank.number == 0 && bank.bytes.size() >= 16 &&
        bank.bytes[0] == 'A' && bank.bytes[1] == 'B' ? 16 : 0;
    while (offset < bank.bytes.size()) {
        const auto decoded = decode_z80(bank.bytes.data() + offset,
            bank.bytes.size() - offset, static_cast<std::uint16_t>(bank.cpu_origin + offset));
        const auto length = std::max<std::size_t>(1, decoded.length);
        if (const auto operand = absolute_operand(bank.bytes, offset, length)) {
            if (*operand >= 0x4000 && *operand < 0xC000) {
                ++references[(*operand - 0x4000) / page_size];
            }
        }
        offset += length;
    }

    const std::size_t fallback_page = (bank.cpu_origin - 0x4000) / page_size;
    std::size_t selected_page = fallback_page;
    for (std::size_t page = 0; page < page_count; ++page) {
        if (references[page] > references[selected_page]) selected_page = page;
    }
    return static_cast<std::uint16_t>(0x4000 + selected_page * page_size);
}

void mark_data(AnalyzedBank& bank, std::size_t offset, ByteKind kind)
{
    bank.states[offset].kind = kind;
}

std::optional<std::pair<std::size_t, std::size_t>> locate_target(
    const RomAnalysis& analysis, std::size_t source_bank, std::uint16_t address)
{
    if (analysis.mapper == MapperType::None) {
        const auto& bank = analysis.banks[source_bank];
        if (address < bank.cpu_origin) return {};
        const std::size_t offset = static_cast<std::size_t>(address - bank.cpu_origin);
        if (offset >= bank.bytes.size()) return {};
        return std::make_pair(source_bank, offset);
    }

    if (address < 0x4000 || address >= 0xC000) return {};
    const std::size_t page_count = analysis.mapper == MapperType::ASCII16 ? 2 : 4;
    const auto& current = analysis.banks[source_bank];
    if (address >= current.cpu_origin &&
        static_cast<std::size_t>(address - current.cpu_origin) < current.bytes.size()) {
        return std::make_pair(source_bank, static_cast<std::size_t>(address - current.cpu_origin));
    }

    std::optional<std::pair<std::size_t, std::size_t>> result;
    const std::size_t group_start = source_bank / page_count * page_count;
    const std::size_t group_end = std::min(group_start + page_count, analysis.banks.size());
    for (std::size_t bank_index = group_start; bank_index < group_end; ++bank_index) {
        const auto& bank = analysis.banks[bank_index];
        if (address < bank.cpu_origin) continue;
        const std::size_t offset = static_cast<std::size_t>(address - bank.cpu_origin);
        if (offset >= bank.bytes.size()) continue;
        if (result) return {};
        result = std::make_pair(bank_index, offset);
    }
    return result;
}

void enqueue_target(RomAnalysis& analysis, std::size_t source_bank, std::uint16_t address,
                    std::deque<WorkItem>& work)
{
    const auto target = locate_target(analysis, source_bank, address);
    if (!target) return;
    // The ROM header is data, so references into it must not restart decoding there.
    if (target->first == 0 && target->second < analysis.header_size) return;
    auto& bank = analysis.banks[target->first];
    bank.states[target->second].has_label = true;
    work.push_back({target->first, target->second});
}

void explore_from(RomAnalysis& analysis, std::deque<WorkItem>& work)
{
    while (!work.empty()) {
        const WorkItem item = work.front();
        work.pop_front();
        auto& bank = analysis.banks[item.bank_index];
        std::size_t offset = item.offset;

        while (offset < bank.bytes.size()) {
            auto& state = bank.states[offset];
            if (state.instruction_length != 0) break;

            const auto address = static_cast<std::uint16_t>(bank.cpu_origin + offset);
            const auto decoded = decode_z80(bank.bytes.data() + offset,
                bank.bytes.size() - offset, address);
            const std::size_t length = std::max<std::size_t>(1,
                std::min(decoded.length, bank.bytes.size() - offset));
            state.kind = ByteKind::Instruction;
            state.instruction_length = static_cast<std::uint8_t>(length);
            for (std::size_t byte = 1; byte < length; ++byte) {
                bank.states[offset + byte].kind = ByteKind::Instruction;
                bank.states[offset + byte].instruction_length = 0;
            }

            if (decoded.target) {
                const auto target = locate_target(analysis, item.bank_index, *decoded.target);
                if (target) {
                    state.target_bank = target->first;
                    enqueue_target(analysis, item.bank_index, *decoded.target, work);
                }
            }

            const bool stop_path = decoded.flow == FlowType::Jump ||
                decoded.flow == FlowType::Return || decoded.flow == FlowType::IndirectJump;
            if (stop_path) break;
            offset += length;
        }
    }
}

void mark_header(AnalyzedBank& bank)
{
    const std::size_t header_size = std::min<std::size_t>(16, bank.bytes.size());
    for (std::size_t offset = 0; offset < header_size; ++offset) {
        mark_data(bank, offset, offset < 2 ? ByteKind::ByteData :
            ((offset & 1) == 0 ? ByteKind::WordLow : ByteKind::WordHigh));
    }
}

// Length of a BIOS entry slot at offset: JP nn, optionally preceded by EI/DI.
std::size_t entry_slot_length(const AnalyzedBank& bank, std::size_t offset)
{
    if (offset >= bank.bytes.size()) return 0;
    const std::size_t prefix =
        (bank.bytes[offset] == 0xFB || bank.bytes[offset] == 0xF3) ? 1 : 0;
    const std::size_t jump = offset + prefix;
    if (jump + 3 > bank.bytes.size() || bank.bytes[jump] != 0xC3) return 0;
    return prefix + 3;
}

bool slot_is_unexamined(const AnalyzedBank& bank, std::size_t offset, std::size_t length)
{
    for (std::size_t byte = 0; byte < length; ++byte) {
        if (bank.states[offset + byte].kind != ByteKind::Unexamined) return false;
    }
    return true;
}

// A run of entry slots padded with NOPs is a BIOS style entry table, even without callers.
void scan_entry_tables(RomAnalysis& analysis, std::deque<WorkItem>& work)
{
    constexpr std::size_t minimum_entries = 3;
    constexpr std::size_t maximum_padding = 5;
    for (std::size_t bank_index = 0; bank_index < analysis.banks.size(); ++bank_index) {
        auto& bank = analysis.banks[bank_index];
        std::size_t offset = 0;
        while (offset < bank.bytes.size()) {
            if (entry_slot_length(bank, offset) == 0) {
                ++offset;
                continue;
            }

            std::vector<std::pair<std::size_t, std::size_t>> entries;
            std::size_t cursor = offset;
            for (std::size_t length = entry_slot_length(bank, cursor); length != 0;
                 length = entry_slot_length(bank, cursor)) {
                entries.emplace_back(cursor, length);
                cursor += length;
                std::size_t padding = 0;
                while (padding < maximum_padding && cursor < bank.bytes.size() &&
                       bank.bytes[cursor] == 0x00) {
                    ++cursor;
                    ++padding;
                }
            }

            if (entries.size() >= minimum_entries) {
                for (const auto& entry : entries) {
                    if (!slot_is_unexamined(bank, entry.first, entry.second)) continue;
                    bank.states[entry.first].has_label = true;
                    work.push_back({bank_index, entry.first});
                }
                offset = cursor;
            } else {
                offset += 1;
            }
        }
    }
}

void normalize_words(AnalyzedBank& bank)
{
    for (std::size_t offset = 0; offset < bank.states.size(); ++offset) {
        if (bank.states[offset].kind == ByteKind::WordLow) {
            if (offset + 1 >= bank.states.size() || bank.states[offset + 1].kind != ByteKind::WordHigh) {
                bank.states[offset].kind = ByteKind::ByteData;
            }
        } else if (bank.states[offset].kind == ByteKind::WordHigh) {
            if (offset == 0 || bank.states[offset - 1].kind != ByteKind::WordLow) {
                bank.states[offset].kind = ByteKind::ByteData;
            }
        }
        if (bank.states[offset].kind == ByteKind::Unexamined) {
            bank.states[offset].kind = ByteKind::ByteData;
        }
    }
}

}

RomAnalysis analyze_rom(const std::vector<std::uint8_t>& rom, MapperType mapper,
                        std::optional<std::uint16_t> origin_override,
                        std::optional<std::uint16_t> entry_override)
{
    if (rom.empty()) throw std::runtime_error("input ROM is empty");

    RomAnalysis analysis;
    analysis.mapper = mapper;
    // "AB" is the standard ROM header id, "CD" is used by the SUB-ROM.
    const bool rom_header = rom.size() >= 16 && rom[0] == 'A' && rom[1] == 'B';
    const bool sub_rom_header = rom.size() >= 16 && rom[0] == 'C' && rom[1] == 'D';
    analysis.has_header = rom_header || sub_rom_header;
    analysis.origin = mapper == MapperType::None ? 0x0100 : 0x4000;
    if (rom_header) analysis.origin = 0x4000;
    if (rom_header && read_word(rom, 2) >= 0x8000) analysis.origin = 0x8000;
    if (sub_rom_header) analysis.origin = 0x0000;
    if (origin_override) analysis.origin = *origin_override;

    const std::size_t bank_size = bank_size_for(mapper, rom.size());
    if (bank_size == 0) throw std::runtime_error("invalid ROM bank size");
    if (mapper == MapperType::None && rom.size() > static_cast<std::size_t>(0x10000u - analysis.origin)) {
        throw std::runtime_error("flat ROM does not fit in the 16-bit address space at the selected origin");
    }

    const std::uint16_t detected_entry = analysis.has_header ? read_word(rom, 2) : analysis.origin;
    analysis.entry = entry_override.value_or(detected_entry);

    if (mapper != MapperType::None) {
        const std::size_t page_size = mapper == MapperType::ASCII16 ? 0x4000 : 0x2000;
        if (analysis.origin < 0x4000 || analysis.origin >= 0xC000 ||
            (analysis.origin - 0x4000) % page_size != 0) {
            throw std::runtime_error("mapper origin must be an aligned address in the 0x4000-0xBFFF ROM area");
        }
    }

    const std::size_t bank_count = (rom.size() + bank_size - 1) / bank_size;
    analysis.banks.reserve(bank_count);
    for (std::size_t bank_number = 0; bank_number < bank_count; ++bank_number) {
        const std::size_t file_offset = bank_number * bank_size;
        const std::size_t length = std::min(bank_size, rom.size() - file_offset);
        AnalyzedBank bank;
        bank.number = bank_number;
        bank.file_offset = file_offset;
        bank.cpu_origin = bank_origin(bank_number, mapper, analysis.origin);
        bank.bytes.assign(rom.begin() + static_cast<std::ptrdiff_t>(file_offset),
            rom.begin() + static_cast<std::ptrdiff_t>(file_offset + length));
        bank.states.resize(length);
        analysis.banks.push_back(std::move(bank));
    }

    if (mapper != MapperType::None) {
        std::optional<std::size_t> entry_bank;
        for (const auto& bank : analysis.banks) {
            if (analysis.entry >= bank.cpu_origin &&
                static_cast<std::size_t>(analysis.entry - bank.cpu_origin) < bank.bytes.size()) {
                entry_bank = bank.number;
                break;
            }
        }
        for (auto& bank : analysis.banks) {
            if (bank.number == entry_bank ||
                (bank.number == 0 && (analysis.has_header || mapper == MapperType::Konami))) continue;
            bank.cpu_origin = infer_bank_origin(bank, mapper);
        }
    }

    if (analysis.has_header && !analysis.banks.empty()) {
        mark_header(analysis.banks[0]);
        analysis.header_size = std::min<std::size_t>(16, analysis.banks[0].bytes.size());
    }

    std::deque<WorkItem> work;
    bool entry_mapped = false;
    for (std::size_t bank_index = 0; bank_index < analysis.banks.size(); ++bank_index) {
        auto& bank = analysis.banks[bank_index];
        if (analysis.entry >= bank.cpu_origin &&
            static_cast<std::size_t>(analysis.entry - bank.cpu_origin) < bank.bytes.size()) {
            const std::size_t offset = static_cast<std::size_t>(analysis.entry - bank.cpu_origin);
            bank.states[offset].has_label = true;
            work.push_back({bank_index, offset});
            entry_mapped = true;
            break;
        }
    }
    if (!entry_mapped) throw std::runtime_error("entry address is outside all ROM banks");

    explore_from(analysis, work);
    scan_entry_tables(analysis, work);
    explore_from(analysis, work);
    for (std::size_t bank_index = 0; bank_index < analysis.banks.size(); ++bank_index) {
        auto& bank = analysis.banks[bank_index];
        const bool entirely_unexamined = std::all_of(bank.states.begin(), bank.states.end(),
            [](const ByteState& state) { return state.kind == ByteKind::Unexamined; });
        if (entirely_unexamined && !bank.bytes.empty()) {
            work.push_back({bank_index, 0});
            explore_from(analysis, work);
        }
    }

    for (auto& bank : analysis.banks) normalize_words(bank);
    return analysis;
}

}