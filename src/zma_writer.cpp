// MIT License
#include "msxdisa/zma_writer.hpp"

#include "msxdisa/z80_decoder.hpp"
#include "msxdisa/msx_symbols.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <string>

namespace msxdisa {
namespace {

std::string format_address(std::uint16_t address)
{
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address;
    return output.str();
}

std::string format_byte(std::uint8_t value)
{
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<unsigned>(value);
    return output.str();
}

std::string make_label(std::size_t bank_number, std::uint16_t address)
{
    std::ostringstream output;
    output << 'B' << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << bank_number
           << 'L' << std::setw(4) << address;
    return output.str();
}

void write_label(std::ostream& output, const AnalyzedBank& bank, std::size_t offset)
{
    if (bank.states[offset].has_label) {
        const auto address = static_cast<std::uint16_t>(bank.cpu_origin + offset);
        output << make_label(bank.number, address) << "::\n";
    }
}

std::string instruction_text(const DecodedInstruction& decoded, const AnalyzedBank& bank,
                             const RomAnalysis& analysis, std::size_t instruction_offset)
{
    std::string text = decoded.text;
    if (decoded.target) {
        if (const auto* symbol = find_bios_symbol(*decoded.target)) {
            const std::string address = format_address(*decoded.target);
            const std::size_t operand = text.rfind(address);
            if (operand != std::string::npos) text.replace(operand, address.size(), symbol->name);
            return text;
        }
    }
    if (!decoded.target || !bank.states[instruction_offset].target_bank) return text;
    const auto target_bank = *bank.states[instruction_offset].target_bank;
    if (target_bank >= analysis.banks.size()) return text;
    const auto& destination = analysis.banks[target_bank];
    if (*decoded.target < destination.cpu_origin) return text;
    const std::size_t target_offset = static_cast<std::size_t>(*decoded.target - destination.cpu_origin);
    if (target_offset >= destination.states.size() || !destination.states[target_offset].has_label) return text;

    const std::string address = format_address(*decoded.target);
    const std::size_t operand = text.rfind(address);
    if (operand != std::string::npos) {
        text.replace(operand, address.size(), make_label(destination.number, *decoded.target));
    }
    return text;
}

std::string instruction_with_work_area_names(std::string text)
{
    std::size_t open = 0;
    while ((open = text.find('[', open)) != std::string::npos) {
        const std::size_t close = text.find(']', open + 1);
        if (close == std::string::npos) break;
        const std::string expression = text.substr(open + 1, close - open - 1);
        if (expression.size() == 6 && expression[0] == '0' &&
            (expression[1] == 'x' || expression[1] == 'X')) {
            const auto address = static_cast<std::uint16_t>(std::stoul(expression, nullptr, 0));
            if (const auto* symbol = find_work_area_symbol(address)) {
                text.replace(open + 1, expression.size(), symbol->name);
                open += std::char_traits<char>::length(symbol->name) + 2;
                continue;
            }
        }
        open = close + 1;
    }
    return text;
}

std::string format_instruction(const std::string& instruction)
{
    const std::size_t separator = instruction.find_first_of(" \t");
    if (separator == std::string::npos) return "    " + instruction;

    const std::string mnemonic = instruction.substr(0, separator);
    const std::size_t operand_start = instruction.find_first_not_of(" \t", separator);
    if (operand_start == std::string::npos) return "    " + mnemonic;

    std::string formatted = "    " + mnemonic;
    std::size_t column = formatted.size();
    do {
        formatted.push_back('\t');
        column = (column / 8 + 1) * 8;
    } while (column < 16);
    formatted += instruction.substr(operand_start);
    return formatted;
}

void add_referenced_symbol(std::map<std::string, std::uint16_t>& symbols, const MsxSymbol* symbol)
{
    if (symbol != nullptr) symbols.emplace(symbol->name, symbol->address);
}

std::map<std::string, std::uint16_t> collect_referenced_symbols(const RomAnalysis& analysis)
{
    std::map<std::string, std::uint16_t> symbols;
    for (const auto& bank : analysis.banks) {
        for (std::size_t offset = 0; offset < bank.bytes.size(); ++offset) {
            if (bank.states[offset].kind != ByteKind::Instruction ||
                bank.states[offset].instruction_length == 0) continue;

            const auto address = static_cast<std::uint16_t>(bank.cpu_origin + offset);
            const auto decoded = decode_z80(bank.bytes.data() + offset,
                bank.bytes.size() - offset, address);
            if (decoded.target) add_referenced_symbol(symbols, find_bios_symbol(*decoded.target));

            std::size_t open = 0;
            while ((open = decoded.text.find('[', open)) != std::string::npos) {
                const std::size_t close = decoded.text.find(']', open + 1);
                if (close == std::string::npos) break;
                const std::string expression = decoded.text.substr(open + 1, close - open - 1);
                if (expression.size() == 6 && expression[0] == '0' &&
                    (expression[1] == 'x' || expression[1] == 'X')) {
                    const auto memory_address = static_cast<std::uint16_t>(std::stoul(expression, nullptr, 0));
                    add_referenced_symbol(symbols, find_work_area_symbol(memory_address));
                }
                open = close + 1;
            }
        }
    }
    return symbols;
}

void write_data_bytes(std::ostream& output, const AnalyzedBank& bank, std::size_t& offset)
{
    const std::size_t begin = offset;
    while (offset < bank.bytes.size() && offset - begin < 8 &&
           (bank.states[offset].kind == ByteKind::ByteData ||
            bank.states[offset].kind == ByteKind::WordHigh ||
            bank.states[offset].kind == ByteKind::Unexamined) &&
           (offset == begin || !bank.states[offset].has_label)) {
        ++offset;
    }

    std::ostringstream data;
    data << "DB ";
    std::string ascii;
    for (std::size_t index = begin; index < offset; ++index) {
        if (index != begin) data << ", ";
        data << format_byte(bank.bytes[index]);
        const auto value = bank.bytes[index];
        ascii.push_back(value >= 33 && value <= 126 ? static_cast<char>(value) : '.');
    }
    output << format_instruction(data.str()) << "    ; " << ascii << "\n";
}

}

void write_zma(std::ostream& output, const RomAnalysis& analysis)
{
    const auto symbols = collect_referenced_symbols(analysis);
    for (const auto& symbol : symbols) {
        output << symbol.first << " := " << format_address(symbol.second) << "\n";
    }
    if (!symbols.empty()) output << "\n";

    for (const auto& bank : analysis.banks) {
        if (analysis.mapper != MapperType::None) {
            output << "; =======================================================\n"
                   << ";  BANK#" << bank.number << "\n";
        }
        output << format_instruction("ORG " + format_address(bank.cpu_origin)) << "\n";
        std::size_t offset = 0;
        while (offset < bank.bytes.size()) {
            const auto address = static_cast<std::uint16_t>(bank.cpu_origin + offset);
            write_label(output, bank, offset);

            const auto& state = bank.states[offset];
            if (state.kind == ByteKind::Instruction && state.instruction_length != 0) {
                const auto decoded = decode_z80(bank.bytes.data() + offset,
                    bank.bytes.size() - offset, address);
                output << format_instruction(instruction_with_work_area_names(
                    instruction_text(decoded, bank, analysis, offset))) << "\n";
                offset += std::max<std::size_t>(1, state.instruction_length);
            } else if (state.kind == ByteKind::WordLow && offset + 1 < bank.bytes.size() &&
                       bank.states[offset + 1].kind == ByteKind::WordHigh) {
                const auto value = static_cast<std::uint16_t>(bank.bytes[offset] |
                    (static_cast<std::uint16_t>(bank.bytes[offset + 1]) << 8));
                output << format_instruction("DW " + format_address(value)) << "\n";
                offset += 2;
            } else if (state.kind == ByteKind::Instruction) {
                output << format_instruction("DB " + format_byte(bank.bytes[offset])) << "\n";
                ++offset;
            } else {
                write_data_bytes(output, bank, offset);
            }
        }
        output << "\n";
    }
}

}