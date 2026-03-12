#pragma once

#include <cstdint>
#include <string_view>

namespace splinter::engine::bytecode {
    class bytecodeTable {
    public:
        [[nodiscard]] static std::string_view name(std::uint8_t opcode) noexcept;
    };
}