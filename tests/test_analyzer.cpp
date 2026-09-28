// MIT License
#include "msxdisa/analyzer.hpp"
#include "msxdisa/zma_writer.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << message << "\n";
        ++failures;
    }
}

}

int main()
{
    std::vector<std::uint8_t> rom(24, 0x00);
    rom[0] = 0x41;
    rom[1] = 0x42;
    rom[2] = 0x04;
    rom[3] = 0x40;
    rom[4] = 0xC3;
    rom[5] = 0x10;
    rom[6] = 0x40;
    rom[16] = 0x00;
    rom[17] = 0xC9;

    const auto analysis = msxdisa::analyze_rom(rom, msxdisa::MapperType::None, {}, {});
    expect(analysis.has_header, "AB header was not detected");
    expect(analysis.origin == 0x4000, "header origin should be 0x4000");
    expect(analysis.entry == 0x4004, "INIT should be the entry address");
    expect(analysis.banks.size() == 1, "flat image should be one bank");

    const auto& bank = analysis.banks[0];
    expect(bank.states[0].kind == msxdisa::ByteKind::ByteData, "header signature byte should be DB data");
    expect(bank.states[1].kind == msxdisa::ByteKind::ByteData, "second signature byte should be DB data");
    expect(bank.states[2].kind == msxdisa::ByteKind::WordLow, "INIT low byte should be word data");
    expect(bank.states[3].kind == msxdisa::ByteKind::WordHigh, "INIT high byte should be word data");
    expect(bank.states[4].kind == msxdisa::ByteKind::Instruction, "INIT target should override header data");
    expect(bank.states[4].has_label, "entry point should be labeled");
    expect(bank.states[16].kind == msxdisa::ByteKind::Instruction, "JP target should be explored");
    expect(bank.states[16].has_label, "JP target should have a label");
    expect(bank.states[17].kind == msxdisa::ByteKind::Instruction, "fallthrough should continue through NOP to RET");
    expect(bank.states[7].kind == msxdisa::ByteKind::ByteData, "orphaned header word half should become byte data");

    std::ostringstream assembly;
    msxdisa::write_zma(assembly, analysis);
    const std::string text = assembly.str();
    expect(text.find("    ORG\t\t0x4000\n") != std::string::npos,
        "ORG should be indented and use the shared operand column");
    expect(text.find("    DB\t\t0x41, 0x42") != std::string::npos,
        "signature should be emitted with an indented DB pseudo-op");
    expect(text.find("    DW\t\t0x4004") != std::string::npos,
        "header word should be emitted with an indented DW pseudo-op");
    expect(text.find("B00L4004::") != std::string::npos, "INIT label should include bank and address");
    expect(text.find("JP\t\tB00L4010") != std::string::npos, "JP operand should use its generated label");
    expect(text.find("B00L4010::") != std::string::npos, "JP target label should be emitted");
    expect(text.find("\n; 0x") == std::string::npos, "standalone address comments should not be emitted");

    std::vector<std::uint8_t> banked_rom(0x4000, 0x00);
    const auto banked = msxdisa::analyze_rom(banked_rom, msxdisa::MapperType::ASCII8, {}, {});
    expect(banked.banks.size() == 2, "ASCII-8 images should be split into 8 KiB banks");
    expect(banked.banks[0].number == 0 && banked.banks[1].number == 1,
        "bank numbers should start at zero and increase");
    expect(banked.banks[0].cpu_origin == 0x4000 && banked.banks[1].cpu_origin == 0x6000,
        "ASCII-8 banks should map to successive 8 KiB windows");
    expect(banked.banks[1].states[0].kind == msxdisa::ByteKind::Instruction,
        "an entirely unexamined bank should be explored from its first byte");

    std::vector<std::uint8_t> cross_bank_rom(0x4000, 0x00);
    cross_bank_rom[0] = 0xC3;
    cross_bank_rom[1] = 0x04;
    cross_bank_rom[2] = 0x60;
    cross_bank_rom[0x2004] = 0x00;
    cross_bank_rom[0x2005] = 0xC9;
    const auto cross_bank = msxdisa::analyze_rom(
        cross_bank_rom, msxdisa::MapperType::ASCII8, {}, {});
    expect(cross_bank.banks[1].states[4].kind == msxdisa::ByteKind::Instruction,
        "JP into the next ASCII-8 window should analyze the corresponding ROM bank");
    expect(cross_bank.banks[1].states[4].has_label,
        "cross-bank JP destination should receive a label");

    std::ostringstream cross_bank_assembly;
    msxdisa::write_zma(cross_bank_assembly, cross_bank);
    expect(cross_bank_assembly.str().find("JP\t\tB01L6004") != std::string::npos,
        "cross-bank JP should use a label with its physical bank number");
    expect(cross_bank_assembly.str().find("B01L6004::") != std::string::npos,
        "cross-bank JP label should be emitted in the destination bank");
    expect(cross_bank_assembly.str().find(";  BANK#1\n    ORG\t\t0x6000") != std::string::npos,
        "bank boundaries should identify the bank before its ORG");

    std::vector<std::uint8_t> window_rom(0x6000, 0xC9);
    window_rom[0] = 0xC3;
    window_rom[1] = 0x06;
    window_rom[2] = 0xA0;
    window_rom[0x2000] = 0x21;
    window_rom[0x2001] = 0x10;
    window_rom[0x2002] = 0xA0;
    window_rom[0x2003] = 0x21;
    window_rom[0x2004] = 0x20;
    window_rom[0x2005] = 0xA0;
    window_rom[0x2006] = 0xC9;
    const auto window_analysis = msxdisa::analyze_rom(
        window_rom, msxdisa::MapperType::ASCII8, {}, {});
    expect(window_analysis.banks[1].cpu_origin == 0xA000,
        "ASCII-8 bank should use its most referenced 8 KiB window");
    expect(window_analysis.banks[0].states[0].target_bank == 1 &&
           window_analysis.banks[1].states[6].has_label,
        "JP should resolve to the bank after its window has been inferred");
    expect(window_analysis.banks[2].cpu_origin == 0x8000,
        "bank without ROM-window references should retain its fallback window");
    const auto explicit_entry = msxdisa::analyze_rom(
        window_rom, msxdisa::MapperType::ASCII8, {}, 0x6000);
    expect(explicit_entry.banks[1].cpu_origin == 0x6000,
        "the bank containing an explicit entry should retain its initial window");
    std::ostringstream window_assembly;
    msxdisa::write_zma(window_assembly, window_analysis);
    expect(window_assembly.str().find(";  BANK#1\n    ORG\t\t0xA000") != std::string::npos,
        "inferred bank window should appear below its boundary comment");
    expect(window_assembly.str().find("JP\t\tB01LA006") != std::string::npos,
        "JP should reference the label in the inferred window");

    std::vector<std::uint8_t> window16_rom(0x8000, 0xC9);
    window16_rom[0x4000] = 0x21;
    window16_rom[0x4001] = 0x10;
    window16_rom[0x4002] = 0x40;
    const auto window16_analysis = msxdisa::analyze_rom(
        window16_rom, msxdisa::MapperType::ASCII16, {}, {});
    expect(window16_analysis.banks[1].cpu_origin == 0x4000,
        "ASCII-16 bank should use its most referenced 16 KiB window");

    const auto konami_analysis = msxdisa::analyze_rom(
        window_rom, msxdisa::MapperType::Konami, {}, {});
    expect(konami_analysis.banks[0].cpu_origin == 0x4000,
        "Konami bank zero must remain in the fixed 0x4000 window");

    std::vector<std::uint8_t> symbol_rom(0xA0, 0x00);
    symbol_rom[0] = 0x2A;
    symbol_rom[1] = 0x4A;
    symbol_rom[2] = 0xFC;
    symbol_rom[3] = 0xCD;
    symbol_rom[4] = 0x9F;
    symbol_rom[5] = 0x00;
    symbol_rom[6] = 0xCD;
    symbol_rom[7] = 0x9A;
    symbol_rom[8] = 0xFD;
    symbol_rom[9] = 0xC9;
    symbol_rom[0x9F] = 0xC9;
    const auto symbol_analysis = msxdisa::analyze_rom(
        symbol_rom, msxdisa::MapperType::None, 0x0000, 0x0000);
    std::ostringstream symbol_assembly;
    msxdisa::write_zma(symbol_assembly, symbol_analysis);
    const auto symbol_text = symbol_assembly.str();
    expect(symbol_text.find("HIMEM := 0xFC4A") != std::string::npos,
        "referenced work-area symbol should be declared at the top");
    expect(symbol_text.find("CHGET := 0x009F") != std::string::npos,
        "referenced BIOS symbol should be declared at the top");
    expect(symbol_text.find("H_KEYI := 0xFD9A") != std::string::npos,
        "referenced hook symbol should be declared at the top");
    expect(symbol_text.find("LD\t\tHL, [HIMEM]") != std::string::npos,
        "absolute memory operand should use its work-area name");
    expect(symbol_text.find("    LD\t\tHL, [HIMEM]") != std::string::npos,
        "instruction mnemonic should be separated from operands by tabs");
    expect(symbol_text.find("CALL\tCHGET") != std::string::npos,
        "BIOS call target should use its routine name");
    expect(symbol_text.find("    CALL\tCHGET") != std::string::npos,
        "longer mnemonics should align operands using the shared tab stop");
    expect(symbol_text.find("CALL\tH_KEYI") != std::string::npos,
        "hook call target should use its hook name");

    const std::vector<std::uint8_t> ascii_rom = {0xC9, 0x41, 0x42, 0x20, 0x7E, 0x7F, 0x80};
    const auto ascii_analysis = msxdisa::analyze_rom(
        ascii_rom, msxdisa::MapperType::None, {}, {});
    std::ostringstream ascii_assembly;
    msxdisa::write_zma(ascii_assembly, ascii_analysis);
    expect(ascii_assembly.str().find("DB\t\t0x41, 0x42, 0x20, 0x7E, 0x7F, 0x80    ; AB.~..")
            != std::string::npos,
        "DB ASCII comment should show printable characters and dots for other bytes");

    return failures == 0 ? 0 : 1;
}