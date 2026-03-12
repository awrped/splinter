#pragma once

#include "../hotspot/constantPool.h"
#include "../hotspot/symbolTable.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace splinter::engine::bytecode {
    struct instructionInfo {
        std::size_t offset = 0;
        std::uint8_t opcode = 0;
        std::string mnemonic;
        std::size_t length = 0;
        std::string operandText;
    };

    class bytecodePrinter {
    public:
        [[nodiscard]] static std::vector<instructionInfo> decode(const std::vector<std::uint8_t> &code);

        [[nodiscard]] static std::vector<instructionInfo> decode(const std::vector<std::uint8_t> &code,
                                                                 const hotspot::constantPoolView &constantPool,
                                                                 const hotspot::symbolTable &symbols);

        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code);

        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code,
                                               const hotspot::constantPoolView &constantPool,
                                               const hotspot::symbolTable &symbols);
    };
}
