// MIT License
#include "msxdisa/z80_decoder.hpp"

#include <array>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(const std::string& name, const std::string& actual, const std::string& expected)
{
    if (actual != expected) {
        std::cerr << name << ": expected '" << expected << "', got '" << actual << "'\n";
        ++failures;
    }
}

void expect_length(const std::string& name, std::size_t actual, std::size_t expected)
{
    if (actual != expected) {
        std::cerr << name << ": expected length " << expected << ", got " << actual << "\n";
        ++failures;
    }
}

}

int main()
{
    const std::array<std::uint8_t, 1> nop = {0x00};
    auto decoded = msxdisa::decode_z80(nop.data(), nop.size(), 0x0100);
    expect("NOP", decoded.text, "NOP");
    expect_length("NOP length", decoded.length, 1);

    const std::array<std::uint8_t, 2> load_immediate = {0x3E, 0x42};
    decoded = msxdisa::decode_z80(load_immediate.data(), load_immediate.size(), 0x0100);
    expect("LD immediate", decoded.text, "LD A, 0x42");
    expect_length("LD immediate length", decoded.length, 2);

    const std::array<std::uint8_t, 3> jump = {0xC3, 0x34, 0x12};
    decoded = msxdisa::decode_z80(jump.data(), jump.size(), 0x0100);
    expect("JP absolute", decoded.text, "JP 0x1234");
    expect_length("JP length", decoded.length, 3);

    const std::array<std::uint8_t, 2> relative = {0x18, 0xFE};
    decoded = msxdisa::decode_z80(relative.data(), relative.size(), 0x0100);
    expect("JR relative", decoded.text, "JR 0x0100");
    expect_length("JR length", decoded.length, 2);

    const std::array<std::uint8_t, 2> bit_test = {0xCB, 0x7E};
    decoded = msxdisa::decode_z80(bit_test.data(), bit_test.size(), 0x0100);
    expect("BIT memory", decoded.text, "BIT 7, [HL]");
    expect_length("BIT length", decoded.length, 2);

    const std::array<std::uint8_t, 2> shift_left = {0xCB, 0x36};
    decoded = msxdisa::decode_z80(shift_left.data(), shift_left.size(), 0x0100);
    expect("SLL memory", decoded.text, "SLL [HL]");
    expect_length("SLL length", decoded.length, 2);

    const std::array<std::uint8_t, 1> rotate_accumulator = {0x07};
    decoded = msxdisa::decode_z80(rotate_accumulator.data(), rotate_accumulator.size(), 0x0100);
    expect("RLCA", decoded.text, "RLCA");

    const std::array<std::uint8_t, 2> block_copy = {0xED, 0xB0};
    decoded = msxdisa::decode_z80(block_copy.data(), block_copy.size(), 0x0100);
    expect("LDIR", decoded.text, "LDIR");
    expect_length("LDIR length", decoded.length, 2);

    const std::array<std::uint8_t, 2> r800_multiply = {0xED, 0xD1};
    decoded = msxdisa::decode_z80(r800_multiply.data(), r800_multiply.size(), 0x0100);
    expect("R800 MULUB", decoded.text,
        "DB 0xED, 0xD1 ; Z80: NOP / R800: MULUB A, D");
    expect_length("R800 MULUB length", decoded.length, 2);

    const std::array<std::uint8_t, 4> load_ix = {0xDD, 0x21, 0x34, 0x12};
    decoded = msxdisa::decode_z80(load_ix.data(), load_ix.size(), 0x0100);
    expect("LD IX immediate", decoded.text, "LD IX, 0x1234");
    expect_length("LD IX length", decoded.length, 4);

    const std::array<std::uint8_t, 3> load_indexed = {0xFD, 0x46, 0xFE};
    decoded = msxdisa::decode_z80(load_indexed.data(), load_indexed.size(), 0x0100);
    expect("LD from indexed memory", decoded.text, "LD B, [IY-2]");
    expect_length("LD indexed length", decoded.length, 3);

    const std::array<std::uint8_t, 4> indexed_bit = {0xDD, 0xCB, 0xFE, 0x46};
    decoded = msxdisa::decode_z80(indexed_bit.data(), indexed_bit.size(), 0x0100);
    expect("BIT indexed memory", decoded.text, "BIT 0, [IX-2]");
    expect_length("BIT indexed length", decoded.length, 4);

    const std::array<std::uint8_t, 1> truncated = {0x3E};
    decoded = msxdisa::decode_z80(truncated.data(), truncated.size(), 0x0100);
    expect("truncated immediate", decoded.text, "DB 0x3E");
    expect_length("truncated immediate length", decoded.length, 1);

    return failures == 0 ? 0 : 1;
}