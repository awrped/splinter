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
        // returns 0 when index is past the end of code, otherwise at least 1
        [[nodiscard]] static std::size_t instructionLength(const std::vector<std::uint8_t> &code, std::size_t index);

        [[nodiscard]] static std::vector<instructionInfo> decode(const std::vector<std::uint8_t> &code);

        // rewritten selects how operands are read. linked classes carry hotspot's
        // rewritten operands (native byte order cache indexes), classes that are
        // only loaded still carry big endian classfile constant pool indexes
        [[nodiscard]] static std::vector<instructionInfo> decode(const std::vector<std::uint8_t> &code,
                                                                 const hotspot::constantPoolView &constantPool,
                                                                 const hotspot::symbolTable &symbols,
                                                                 bool rewritten = true);

        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code);

        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code,
                                               const hotspot::constantPoolView &constantPool,
                                               const hotspot::symbolTable &symbols,
                                               bool rewritten = true);
    };
}
