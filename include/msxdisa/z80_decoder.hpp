// MIT License
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace msxdisa {

enum class FlowType {
    Fallthrough,
    ConditionalBranch,
    Jump,
    Call,
    ConditionalCall,
    ConditionalReturn,
    Return,
    IndirectJump
};

struct DecodedInstruction {
    std::size_t length;
    std::string text;
    FlowType flow = FlowType::Fallthrough;
    std::optional<std::uint16_t> target;
};

DecodedInstruction decode_z80(const std::uint8_t* bytes, std::size_t size, std::uint16_t address);

}