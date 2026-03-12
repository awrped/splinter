#pragma once

#include "../memory/processMemory.h"
#include "vmStructs.h"

#include <cstdint>
#include <string>

namespace splinter::engine::hotspot {
    class symbolTable {
    public:
        symbolTable(const memory::processMemory &memory, const vmStructs &vm) noexcept;

        [[nodiscard]] std::string readSymbol(std::uint64_t symbolAddress) const;

    private:
        memory::processMemory memory_;
        const vmStructs *vm_ = nullptr;
    };
}
