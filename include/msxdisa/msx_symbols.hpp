// MIT License
#pragma once

#include <array>
#include <cstdint>

namespace msxdisa {

struct MsxSymbol {
    std::uint16_t address;
    const char* name;
};

inline constexpr MsxSymbol bios_symbols[] = {
    MsxSymbol{0x0000, "CHKRAM"}, {0x0008, "SYNCHR"}, {0x000C, "RDSLT"},
    {0x0010, "CHRGTR"}, {0x0014, "WRSLT"}, {0x0018, "OUTDO"},
    {0x001C, "CALSLT"}, {0x0020, "DCOMPR"}, {0x0024, "ENASLT"},
    {0x0028, "GETYPR"}, {0x0030, "CALLF"}, {0x0038, "KEYINT"},
    {0x003B, "INITIO"}, {0x003E, "INIFNK"}, {0x0041, "DISSCR"},
    {0x0044, "ENASCR"}, {0x0047, "WRTVDP"}, {0x004A, "RDVRM"},
    {0x004D, "WRTVRM"}, {0x0050, "SETRD"}, {0x0053, "SETWRT"},
    {0x0056, "FILVRM"}, {0x0059, "LDIRMV"}, {0x005C, "LDIRVM"},
    {0x005F, "CHGMOD"}, {0x0062, "CHGCLR"}, {0x0066, "NMI"},
    {0x0069, "CLRSPR"}, {0x006C, "INITXT"}, {0x006F, "INIT32"},
    {0x0072, "INIGRP"}, {0x0075, "INIMLT"}, {0x0078, "SETTXT"},
    {0x007B, "SETT32"}, {0x007E, "SETGRP"}, {0x0081, "SETMLT"},
    {0x0084, "CALPAT"}, {0x0087, "CALATR"}, {0x008A, "GSPSIZ"},
    {0x008D, "GRPPRT"}, {0x0090, "GICINI"}, {0x0093, "WRTPSG"},
    {0x0096, "RDPSG"}, {0x0099, "STRTMS"}, {0x009C, "CHSNS"},
    {0x009F, "CHGET"}, {0x00A2, "CHPUT"}, {0x00A5, "LPTOUT"},
    {0x00A8, "LPTSTT"}, {0x00AB, "CNVCHR"}, {0x00AE, "PINLIN"},
    {0x00B1, "INLIN"}, {0x00B4, "QINLIN"}, {0x00B7, "BREAKX"},
    {0x00C0, "BEEP"}, {0x00C3, "CLS"}, {0x00C6, "POSIT"},
    {0x00C9, "FNKSB"}, {0x00CC, "ERAFNK"}, {0x00CF, "DSPFNK"},
    {0x00D2, "TOTEXT"}, {0x00D5, "GTSTCK"}, {0x00D8, "GTTRIG"},
    {0x00DB, "GTPAD"}, {0x00DE, "GTPDL"}, {0x00E1, "TAPION"},
    {0x00E4, "TAPIN"}, {0x00E7, "TAPIOF"}, {0x00EA, "TAPOON"},
    {0x00ED, "TAPOUT"}, {0x00F0, "TAPOOF"}, {0x00F3, "STMOTR"},
    {0x0132, "CHGCAP"}, {0x0135, "CHGSND"}, {0x0138, "RSLREG"},
    {0x013B, "WSLREG"}, {0x013E, "RDVDP"}, {0x0141, "SNSMAT"},
    {0x0144, "PHYDIO"}, {0x0147, "FORMAT"}, {0x014A, "ISFLIO"},
    {0x014D, "OUTDLP"}, {0x0156, "KILBUF"}, {0x0159, "CALBAS"},
    {0x015C, "SUBROM"}, {0x015F, "EXTROM"}, {0x0168, "EOL"},
    {0x016B, "BIGFIL"}, {0x016E, "NSETRD"}, {0x0171, "NSTWRT"},
    {0x0174, "NRDVRM"}, {0x0177, "NWRVRM"}
};

inline constexpr MsxSymbol work_area_symbols[] = {
    MsxSymbol{0xF380, "RDPRIM"}, {0xF385, "WRPRIM"}, {0xF38C, "CLPRIM"},
    {0xF3AE, "LINL40"}, {0xF3AF, "LINL32"}, {0xF3B0, "LINLEN"},
    {0xF3B3, "TXTNAM"}, {0xF3B7, "TXTCGP"}, {0xF3BD, "T32NAM"},
    {0xF3BF, "T32COL"}, {0xF3C1, "T32CGP"}, {0xF3C3, "T32ATR"},
    {0xF3C5, "T32PAT"}, {0xF3C7, "GRPNAM"}, {0xF3C9, "GRPCOL"},
    {0xF3CB, "GRPCGP"}, {0xF3CD, "GRPATR"}, {0xF3CF, "GRPPAT"},
    {0xF3D1, "MLTNAM"}, {0xF3D3, "MLTCOL"}, {0xF3D5, "MLTCGP"},
    {0xF3D7, "MLTATR"}, {0xF3D9, "MLTPAT"}, {0xF3DB, "CLIKSW"},
    {0xF3DC, "CSRY"}, {0xF3DD, "CSRX"}, {0xF3DE, "CNSDFG"},
    {0xF3DF, "RG0SAV"}, {0xF3E0, "RG1SAV"}, {0xF3E1, "RG2SAV"},
    {0xF3E2, "RG3SAV"}, {0xF3E3, "RG4SAV"}, {0xF3E4, "RG5SAV"},
    {0xF3E5, "RG6SAV"}, {0xF3E6, "RG7SAV"}, {0xF3E7, "STATFL"},
    {0xF3E9, "FORCLR"}, {0xF3EA, "BAKCLR"}, {0xF3EB, "BDRCLR"},
    {0xF348, "MASTERS"}, {0xF341, "RAMAD0"}, {0xF342, "RAMAD1"},
    {0xF343, "RAMAD2"}, {0xF344, "RAMAD3"}, {0xF323, "DISKVE"},
    {0xF325, "BREAKV"}, {0xF87F, "FNKSTR"}, {0xFAF8, "EXBRSA"},
    {0xFB20, "HOKVLD"}, {0xFC48, "BOTTOM"}, {0xFC4A, "HIMEM"},
    {0xFC4C, "TRPTBL"}, {0xFCAF, "SCRMOD"}, {0xFCC1, "EXPTBL"},
    {0xFCC5, "SLTTBL"}, {0xFCC9, "SLTATR"}, {0xFD09, "SLTWRK"},
    {0xFD89, "PROCNM"}, {0xFD99, "DEVICE"}, {0xF24F, "H_PROMPT"},
    {0xFD9A, "H_KEYI"}, {0xFD9F, "H_TIMI"}, {0xFDA4, "H_CHPH"},
    {0xFDA9, "H_DSPC"}, {0xFDAE, "H_ERAC"}, {0xFDB3, "H_DSPF"},
    {0xFDB8, "H_ERAF"}, {0xFDBD, "H_TOTE"}, {0xFDC2, "H_CHGE"},
    {0xFDC7, "H_INIP"}, {0xFDCC, "H_KEYC"}, {0xFDD1, "H_KYEA"},
    {0xFDD6, "H_NMI"}, {0xFDDB, "H_PINL"}, {0xFDE0, "H_QINL"},
    {0xFDE5, "H_INLI"}, {0xFDEA, "H_ONGO"}, {0xFDEF, "H_DSKO"},
    {0xFDF4, "H_SETS"}, {0xFDF9, "H_NAME"}, {0xFDFE, "H_KILL"},
    {0xFE03, "H_IPL"}, {0xFE08, "H_COPY"}, {0xFE0D, "H_CMD"},
    {0xFE12, "H_DSKF"}, {0xFE17, "H_DSKI"}, {0xFE1C, "H_ATTR"},
    {0xFE21, "H_LSET"}, {0xFE26, "H_RSET"}, {0xFE2B, "H_FIEL"},
    {0xFE30, "H_MKI"}, {0xFE35, "H_MKS"}, {0xFE3A, "H_MKD"},
    {0xFE3F, "H_CVI"}, {0xFE44, "H_CVS"}, {0xFE49, "H_CVD"},
    {0xFE4E, "H_GETP"}, {0xFE53, "H_SETF"}, {0xFE58, "H_NOFO"},
    {0xFE5D, "H_NULO"}, {0xFE62, "H_NTFL"}, {0xFE67, "H_MERG"},
    {0xFE6C, "H_SAVE"}, {0xFE71, "H_BINS"}, {0xFE76, "H_BINL"},
    {0xFE7B, "H_FILE"}, {0xFE80, "H_DGET"}, {0xFE85, "H_FILO"},
    {0xFE8A, "H_INDS"}, {0xFE8F, "H_RSLF"}, {0xFE94, "H_SAVD"},
    {0xFE99, "H_LOC"}, {0xFE9E, "H_LOF"}, {0xFEA3, "H_EOF"},
    {0xFEA8, "H_FPOS"}, {0xFEAD, "H_BAKU"}, {0xFEB2, "H_PARD"},
    {0xFEB7, "H_NODE"}, {0xFEBC, "H_POSD"}, {0xFEC1, "H_DEVN"},
    {0xFEC6, "H_GEND"}, {0xFECB, "H_RUNC"}, {0xFED0, "H_CLEA"},
    {0xFED5, "H_LOPD"}, {0xFEDA, "H_STKE"}, {0xFEDF, "H_ISFL"},
    {0xFEE4, "H_OUTD"}, {0xFEE9, "H_CRDO"}, {0xFEEE, "H_DSKC"},
    {0xFEF3, "H_DOGR"}, {0xFEF8, "H_PRGE"}, {0xFEFD, "H_ERRP"},
    {0xFF02, "H_ERRF"}, {0xFF07, "H_READ"}, {0xFF0C, "H_MAIN"},
    {0xFF11, "H_DIRD"}, {0xFF16, "H_FINI"}, {0xFF1B, "H_FINE"},
    {0xFF20, "H_CRUN"}, {0xFF25, "H_CRUS"}, {0xFF2A, "H_ISRE"},
    {0xFF2F, "H_NTFN"}, {0xFF34, "H_NOTR"}, {0xFF39, "H_SNGF"},
    {0xFF3E, "H_NEWS"}, {0xFF43, "H_GONE"}, {0xFF48, "H_CHRG"},
    {0xFF4D, "H_RETU"}, {0xFF52, "H_PRTF"}, {0xFF57, "H_COMP"},
    {0xFF5C, "H_FINP"}, {0xFF61, "H_TRMN"}, {0xFF66, "H_FRME"},
    {0xFF6B, "H_NTPL"}, {0xFF70, "H_EVAL"}, {0xFF75, "H_OKNO"},
    {0xFF7A, "H_FING"}, {0xFF7F, "H_ISMI"}, {0xFF84, "H_WIDT"},
    {0xFF89, "H_LIST"}, {0xFF8E, "H_BUFL"}, {0xFF93, "H_FRQI"},
    {0xFF98, "H_SCNE"}, {0xFF9D, "H_FRET"}, {0xFFA2, "H_PTRG"},
    {0xFFA7, "H_PHYD"}, {0xFFAC, "H_FORM"}, {0xFFB1, "H_ERRO"},
    {0xFFB6, "H_LPTO"}, {0xFFBB, "H_LPTS"}, {0xFFC0, "H_SCRE"},
    {0xFFC5, "H_PLAY"}, {0xFFCA, "EXTBIO"}
};

template <std::size_t Size>
inline const MsxSymbol* find_symbol(const MsxSymbol (&symbols)[Size], std::uint16_t address)
{
    for (const auto& symbol : symbols) {
        if (symbol.address == address) return &symbol;
    }
    return nullptr;
}

inline const MsxSymbol* find_bios_symbol(std::uint16_t address)
{
    if (const auto* symbol = find_symbol(bios_symbols, address)) return symbol;
    const auto* symbol = find_symbol(work_area_symbols, address);
    if (symbol != nullptr && ((symbol->name[0] == 'H' && symbol->name[1] == '_') ||
                              symbol->address == 0xFFCA)) {
        return symbol;
    }
    return nullptr;
}

inline const MsxSymbol* find_work_area_symbol(std::uint16_t address)
{
    return find_symbol(work_area_symbols, address);
}

}