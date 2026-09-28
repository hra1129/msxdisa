# msxdisa

A portable C++17 command-line disassembler that emits ZMA-style assembly source.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Current support

The analyzer tracks each byte as unexamined, byte data, a low/high word byte, or instruction, and separately tracks labels. For a 16-byte `AB` header, the signature is emitted as bytes and the remaining header fields as words; execution analysis can override header data where INIT points. It follows direct JP/JR/CALL targets and fallthrough until unconditional JP/RET, then starts from the first byte of banks that remain entirely unexamined. Unmatched word halves are converted to byte data.

DB lines include an ASCII comment: bytes `0x21` through `0x7E` are shown as characters and all other bytes are shown as `.`. Direct references to known MSX BIOS routines and hooks use their symbol names; absolute memory references to known system work areas use their work-area names. Only referenced symbols are declared at the top of the output.

`ascii16` splits images into 16 KiB banks with windows at `0x4000` and `0x8000`; `ascii8`, `konami`, and `scc` use 8 KiB banks with windows at `0x4000`, `0x6000`, `0x8000`, and `0xA000`. Bank numbering starts at zero. Each bank's 16-bit instruction operands are counted by referenced ROM window; the most referenced window determines its ORG. A tie or no references retains the sequential default. The entry bank and Konami's fixed bank zero retain their initial window. Each bank begins with a `BANK#` comment above its ORG. Labels use the form `B00L4004`, with the bank number and CPU address rendered in uppercase hexadecimal. The decoder currently covers base, CB, ED, and a subset of IX/IY instructions; unknown encodings are emitted as byte data. Direct targets are explored when their window resolves to a unique bank in the same sequential group; runtime mapper-register writes are not emulated.

```text
msxdisa [--origin address] [--start address] [-o output.asm] rom.rom
```

Flat images default to origin `0x0100`; `AB` cartridge images default to `0x4000`, or `0x8000` when INIT is at or above `0x8000`. `--origin` and `--start` override those values. `--mapper` accepts `none`, `ascii16`, `ascii8`, `konami`, and `scc`. Control-flow analysis is heuristic: arbitrary embedded data may still be interpreted as instructions when reached or when an entirely unexamined bank is scanned from its beginning. The current built-in symbol table covers common BIOS entries, work areas, and hooks; runtime bank-switch emulation is not implemented yet.

The built-in MSX symbols are based on [TechHan Appendix A.1](https://ngs.no.coocan.jp/doc/wiki.cgi/TechHan?page=Appendix+A%2E1+BIOS+%B0%EC%CD%F7) and [TechHan Appendix A.4](https://ngs.no.coocan.jp/doc/wiki.cgi/TechHan?page=Appendix+A%2E4+%A5%EF%A1%BC%A5%AF%A5%A8%A5%EA%A5%A2%B0%EC%CD%F7).