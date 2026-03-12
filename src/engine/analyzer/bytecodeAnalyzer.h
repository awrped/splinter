#pragma once

#include "../bytecode/bytecodePrinter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace splinter::engine::analyzer {
    class bytecodeAnalyzer {
    public:
        [[nodiscard]] std::string summarize(const std::vector<std::uint8_t> &code) const;
    };
}