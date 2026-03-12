#pragma once

#include "../hotspot/constantPool.h"
#include "../hotspot/symbolTable.h"

#include <cstdint>
#include <string>
#include <vector>

namespace splinter::engine::bytecode {
    class bytecodePrinter {
    public:
        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code);

        [[nodiscard]] static std::string print(const std::vector<std::uint8_t> &code,
                                               const hotspot::constantPoolView &constantPool,
                                               const hotspot::symbolTable &symbols);
    };
}