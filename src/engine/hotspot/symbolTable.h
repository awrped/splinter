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

        // hotspot's preallocated symbol table, injected fields name themselves with
        // an index into this instead of a constant pool index
        [[nodiscard]] std::string readVmSymbol(std::uint32_t symbolId) const;

    private:
        memory::processMemory memory_;
        const vmStructs *vm_ = nullptr;
    };
}