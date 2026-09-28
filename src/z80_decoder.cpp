// MIT License
#include "msxdisa/z80_decoder.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <utility>

namespace msxdisa {
namespace {

const std::array<const char*, 8> registers = {"B", "C", "D", "E", "H", "L", "[HL]", "A"};
const std::array<const char*, 4> register_pairs = {"BC", "DE", "HL", "SP"};
const std::array<const char*, 4> stack_pairs = {"BC", "DE", "HL", "AF"};
const std::array<const char*, 8> conditions = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
const std::array<const char*, 8> alu_names = {"ADD A", "ADC A", "SUB A", "SBC A", "AND", "XOR", "OR", "CP"};

std::string hex_value(std::uint16_t value, unsigned digits = 4)
{
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(static_cast<int>(digits)) << value;
    return output.str();
}

std::string byte_value(std::uint8_t value)
{
    return hex_value(value, 2);
}

std::uint16_t read_word(const std::uint8_t* bytes)
{
    return static_cast<std::uint16_t>(bytes[0] | (static_cast<std::uint16_t>(bytes[1]) << 8));
}

DecodedInstruction raw_byte(std::uint8_t value)
{
    return {1, "DB " + byte_value(value)};
}

DecodedInstruction decode_cb(const std::uint8_t* bytes, std::size_t size)
{
    if (size < 2) return raw_byte(bytes[0]);

    const std::uint8_t opcode = bytes[1];
    const unsigned group = opcode >> 6;
    const unsigned operation = (opcode >> 3) & 7;
    const unsigned target = opcode & 7;
    static const std::array<const char*, 8> rotations = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};

    std::string text;
    if (group == 0) {
        text = std::string(rotations[operation]) + " " + registers[target];
    } else if (group == 1) {
        text = "BIT " + std::to_string(operation) + ", " + registers[target];
    } else if (group == 2) {
        text = "RES " + std::to_string(operation) + ", " + registers[target];
    } else {
        text = "SET " + std::to_string(operation) + ", " + registers[target];
    }
    return {2, std::move(text)};
}

DecodedInstruction decode_ed(const std::uint8_t* bytes, std::size_t size)
{
    if (size < 2) return raw_byte(bytes[0]);

    const std::uint8_t opcode = bytes[1];
    const unsigned register_index = (opcode >> 3) & 7;
    static const std::array<const char*, 8> input_registers = {"B", "C", "D", "E", "H", "L", "", "A"};
    static const std::array<const char*, 4> pairs = {"BC", "DE", "HL", "SP"};

    if ((opcode & 0xC7) == 0x40 && register_index != 6) {
        return {2, "IN " + std::string(input_registers[register_index]) + ", [C]"};
    }
    if ((opcode & 0xC7) == 0x41 && register_index != 6) {
        return {2, "OUT [C], " + std::string(input_registers[register_index])};
    }
    if ((opcode & 0xCF) == 0x42) {
        const auto operation = (opcode & 0x08) == 0 ? "SBC HL, " : "ADC HL, ";
        return {2, std::string(operation) + pairs[(opcode >> 4) & 3]};
    }
    if ((opcode & 0xCF) == 0x43) {
        if (size < 4) return {2, "DB " + byte_value(bytes[0]) + ", " + byte_value(bytes[1])};
        const auto address = hex_value(read_word(bytes + 2));
        const auto pair = pairs[(opcode >> 4) & 3];
        if ((opcode & 0x08) == 0) return {4, "LD [" + address + "], " + pair};
        return {4, "LD " + std::string(pair) + ", [" + address + "]"};
    }
    if (opcode == 0x44) return {2, "NEG"};
    if (opcode == 0x45) return {2, "RETN"};
    if (opcode == 0x4D) return {2, "RETI"};
    if (opcode == 0x46 || opcode == 0x56 || opcode == 0x5E) {
        const unsigned mode = opcode == 0x46 ? 0 : (opcode == 0x56 ? 1 : 2);
        return {2, "IM" + std::to_string(mode)};
    }
    if (opcode == 0x47) return {2, "LD I, A"};
    if (opcode == 0x4F) return {2, "LD R, A"};
    if (opcode == 0x57) return {2, "LD A, I"};
    if (opcode == 0x5F) return {2, "LD A, R"};
    if (opcode == 0x67) return {2, "RRD"};
    if (opcode == 0x6F) return {2, "RLD"};

    static const std::array<const char*, 28> block_operations = {
        "LDI", "CPI", "INI", "OUTI", "", "", "", "",
        "LDD", "CPD", "IND", "OUTD", "", "", "", "",
        "LDIR", "CPIR", "INIR", "OTIR", "", "", "", "",
        "LDDR", "CPDR", "INDR", "OTDR"
    };
    if (opcode >= 0xA0 && opcode <= 0xBB) {
        const char* mnemonic = block_operations[opcode & 0x1F];
        if (mnemonic[0] != '\0') return {2, mnemonic};
    }

    if ((opcode & 0xC7) == 0xC1 && register_index < 4) {
        static const std::array<const char*, 4> mulub_registers = {"B", "C", "D", "E"};
        return {2, "DB " + byte_value(bytes[0]) + ", " + byte_value(opcode) +
            " ; Z80: NOP / R800: MULUB A, " + mulub_registers[register_index]};
    }
    if (opcode == 0xC3 || opcode == 0xC5) {
        const char* pair = opcode == 0xC3 ? "BC" : "SP";
        return {2, "DB " + byte_value(bytes[0]) + ", " + byte_value(opcode) +
            " ; Z80: NOP / R800: MULUW HL, " + pair};
    }

    return {2, "DB " + byte_value(bytes[0]) + ", " + byte_value(opcode)};
}

DecodedInstruction decode_base(const std::uint8_t* bytes, std::size_t size, std::uint16_t address);

std::string indexed_memory(const char* index_register, std::uint8_t displacement)
{
    const auto signed_displacement = static_cast<std::int8_t>(displacement);
    if (signed_displacement == 0) return "[" + std::string(index_register) + "]";
    if (signed_displacement < 0) {
        return "[" + std::string(index_register) + "-" + std::to_string(-signed_displacement) + "]";
    }
    return "[" + std::string(index_register) + "+" + std::to_string(signed_displacement) + "]";
}

void replace_all(std::string& text, const std::string& from, const std::string& to)
{
    std::size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
}

DecodedInstruction decode_indexed(const std::uint8_t* bytes, std::size_t size,
                                  std::uint16_t address, const char* index_register)
{
    if (size < 2) return raw_byte(bytes[0]);
    const std::uint8_t opcode = bytes[1];

    if (opcode == 0xCB) {
        if (size < 4) return raw_byte(bytes[0]);
        const std::uint8_t indexed_opcode = bytes[3];
        const unsigned group = indexed_opcode >> 6;
        if ((group != 1 && (indexed_opcode & 7) != 6)) {
            return {4, "DB " + byte_value(bytes[0]) + ", " + byte_value(bytes[1]) + ", " +
                byte_value(bytes[2]) + ", " + byte_value(bytes[3])};
        }
        const std::array<std::uint8_t, 2> normalized = {
            0xCB, static_cast<std::uint8_t>((indexed_opcode & 0xF8) | 6)
        };
        auto decoded = decode_cb(normalized.data(), normalized.size());
        replace_all(decoded.text, "[HL]", indexed_memory(index_register, bytes[2]));
        return {4, std::move(decoded.text)};
    }

    if (opcode == 0x36) {
        if (size < 4) return raw_byte(bytes[0]);
        return {4, "LD " + indexed_memory(index_register, bytes[2]) + ", " + byte_value(bytes[3])};
    }

    const unsigned group = opcode >> 6;
    const unsigned y = (opcode >> 3) & 7;
    const unsigned z = opcode & 7;
    const bool indexed_memory_operand =
        (opcode == 0x34 || opcode == 0x35) ||
        (group == 1 && opcode != 0x76 && (y == 6 || z == 6)) ||
        (group == 2 && z == 6);
    if (indexed_memory_operand) {
        if (size < 3) return raw_byte(bytes[0]);
        auto decoded = decode_base(bytes + 1, size - 1, static_cast<std::uint16_t>(address + 1));
        replace_all(decoded.text, "[HL]", indexed_memory(index_register, bytes[2]));
        return {decoded.length + 2, std::move(decoded.text)};
    }

    const bool index_pair_instruction =
        opcode == 0x09 || opcode == 0x19 || opcode == 0x21 || opcode == 0x22 ||
        opcode == 0x23 || opcode == 0x29 || opcode == 0x2A || opcode == 0x2B ||
        opcode == 0x39 || opcode == 0xE1 || opcode == 0xE3 || opcode == 0xE5 ||
        opcode == 0xE9 || opcode == 0xF9;
    if (index_pair_instruction) {
        auto decoded = decode_base(bytes + 1, size - 1, static_cast<std::uint16_t>(address + 1));
        if (decoded.text.rfind("DB ", 0) == 0) return raw_byte(bytes[0]);
        replace_all(decoded.text, "HL", index_register);
        return {decoded.length + 1, std::move(decoded.text)};
    }

    return raw_byte(bytes[0]);
}

DecodedInstruction decode_base(const std::uint8_t* bytes, std::size_t size, std::uint16_t address)
{
    const std::uint8_t opcode = bytes[0];
    const unsigned group = opcode >> 6;
    const unsigned y = (opcode >> 3) & 7;
    const unsigned z = opcode & 7;
    const unsigned pair = y >> 1;
    const bool alternate = (y & 1) != 0;

    if (group == 0) {
        if (z == 0) {
            if (y == 0) return {1, "NOP"};
            if (y == 1) return {1, "EX AF, AF'"};
            if (size < 2) return raw_byte(opcode);
            const auto displacement = static_cast<std::int8_t>(bytes[1]);
            const auto target = static_cast<std::uint16_t>(address + 2 + displacement);
            if (y == 2) return {2, "DJNZ " + hex_value(target)};
            if (y == 3) return {2, "JR " + hex_value(target)};
            return {2, "JR " + std::string(conditions[y - 4]) + ", " + hex_value(target)};
        }
        if (z == 1) {
            if (!alternate) {
                if (size < 3) return raw_byte(opcode);
                return {3, "LD " + std::string(register_pairs[pair]) + ", " + hex_value(read_word(bytes + 1))};
            }
            return {1, "ADD HL, " + std::string(register_pairs[pair])};
        }
        if (z == 2) {
            if (y < 4) {
                const char* pair_name = y < 2 ? "BC" : "DE";
                if ((y & 1) == 0) return {1, "LD [" + std::string(pair_name) + "], A"};
                return {1, "LD A, [" + std::string(pair_name) + "]"};
            }
            if (size < 3) return raw_byte(opcode);
            const auto absolute = hex_value(read_word(bytes + 1));
            if (y == 4) return {3, "LD [" + absolute + "], HL"};
            if (y == 5) return {3, "LD HL, [" + absolute + "]"};
            if (y == 6) return {3, "LD [" + absolute + "], A"};
            return {3, "LD A, [" + absolute + "]"};
        }
        if (z == 3) return {1, std::string(alternate ? "DEC " : "INC ") + register_pairs[pair]};
        if (z == 4 || z == 5) return {1, std::string(z == 4 ? "INC " : "DEC ") + registers[y]};
        if (z == 7) {
            static const std::array<const char*, 8> accumulator_operations = {
                "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF"
            };
            return {1, accumulator_operations[y]};
        }
        if (size < 2) return raw_byte(opcode);
        return {2, "LD " + std::string(registers[y]) + ", " + byte_value(bytes[1])};
    }

    if (group == 1) {
        if (opcode == 0x76) return {1, "HALT"};
        return {1, "LD " + std::string(registers[y]) + ", " + registers[z]};
    }

    if (group == 2) {
        std::string text = alu_names[y];
        if (y < 4) text += ",";
        text += " " + std::string(registers[z]);
        return {1, std::move(text)};
    }

    if (z == 0) return {1, "RET " + std::string(conditions[y])};
    if (z == 1) {
        if (!alternate) return {1, "POP " + std::string(stack_pairs[pair])};
        if (pair == 0) return {1, "RET"};
        if (pair == 1) return {1, "EXX"};
        if (pair == 2) return {1, "JP HL"};
        return {1, "LD SP, HL"};
    }
    if (z == 2) {
        if (size < 3) return raw_byte(opcode);
        return {3, "JP " + std::string(conditions[y]) + ", " + hex_value(read_word(bytes + 1))};
    }
    if (z == 3) {
        if (y == 0) {
            if (size < 3) return raw_byte(opcode);
            return {3, "JP " + hex_value(read_word(bytes + 1))};
        }
        if (y == 2) {
            if (size < 2) return raw_byte(opcode);
            return {2, "OUT [" + byte_value(bytes[1]) + "], A"};
        }
        if (y == 3) {
            if (size < 2) return raw_byte(opcode);
            return {2, "IN A, [" + byte_value(bytes[1]) + "]"};
        }
        if (y == 4) return {1, "EX [SP], HL"};
        if (y == 5) return {1, "EX DE, HL"};
        if (y == 6) return {1, "DI"};
        if (y == 7) return {1, "EI"};
        return raw_byte(opcode);
    }
    if (z == 4) {
        if (size < 3) return raw_byte(opcode);
        return {3, "CALL " + std::string(conditions[y]) + ", " + hex_value(read_word(bytes + 1))};
    }
    if (z == 5) {
        if (!alternate) return {1, "PUSH " + std::string(stack_pairs[pair])};
        if (pair == 0) {
            if (size < 3) return raw_byte(opcode);
            return {3, "CALL " + hex_value(read_word(bytes + 1))};
        }
        return raw_byte(opcode);
    }
    if (z == 6) {
        if (size < 2) return raw_byte(opcode);
        std::string text = alu_names[y];
        if (y < 4) text += ",";
        text += " " + byte_value(bytes[1]);
        return {2, std::move(text)};
    }
    return {1, "RST " + hex_value(static_cast<std::uint16_t>(y * 8))};
}

}

DecodedInstruction decode_z80(const std::uint8_t* bytes, std::size_t size, std::uint16_t address)
{
    if (size == 0) return {0, {}};
    DecodedInstruction decoded;
    if (bytes[0] == 0xCB) {
        decoded = decode_cb(bytes, size);
    } else if (bytes[0] == 0xED) {
        decoded = decode_ed(bytes, size);
        if (size >= 2 && (bytes[1] == 0x45 || bytes[1] == 0x4D)) decoded.flow = FlowType::Return;
    } else if (bytes[0] == 0xDD) {
        decoded = decode_indexed(bytes, size, address, "IX");
    } else if (bytes[0] == 0xFD) {
        decoded = decode_indexed(bytes, size, address, "IY");
    } else {
        decoded = decode_base(bytes, size, address);
    }

    const std::uint8_t opcode = bytes[0];
    if (opcode == 0x10 || opcode == 0x18 ||
        (opcode == 0x20 || opcode == 0x28 || opcode == 0x30 || opcode == 0x38)) {
        if (size >= 2) {
            const auto displacement = static_cast<std::int8_t>(bytes[1]);
            decoded.target = static_cast<std::uint16_t>(address + decoded.length + displacement);
            decoded.flow = opcode == 0x18 ? FlowType::Jump : FlowType::ConditionalBranch;
        }
    } else if (opcode == 0xC3 || (opcode & 0xC7) == 0xC2) {
        if (size >= 3) {
            decoded.target = read_word(bytes + 1);
            decoded.flow = opcode == 0xC3 ? FlowType::Jump : FlowType::ConditionalBranch;
        }
    } else if (opcode == 0xCD || (opcode & 0xC7) == 0xC4) {
        if (size >= 3) {
            decoded.target = read_word(bytes + 1);
            decoded.flow = opcode == 0xCD ? FlowType::Call : FlowType::ConditionalCall;
        }
    } else if (opcode == 0xC9) {
        decoded.flow = FlowType::Return;
    } else if ((opcode & 0xC7) == 0xC0) {
        decoded.flow = FlowType::ConditionalReturn;
    } else if (opcode == 0xE9 || ((opcode == 0xDD || opcode == 0xFD) && size >= 2 && bytes[1] == 0xE9)) {
        decoded.flow = FlowType::IndirectJump;
    }
    return decoded;
}

}