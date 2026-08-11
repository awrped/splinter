#pragma once

#include "../memory/processMemory.h"
#include "vmStructs.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace splinter::engine::hotspot {
    class symbolTable {
    public:
        symbolTable(const memory::processMemory &memory, const vmStructs &vm) noexcept;

        [[nodiscard]] std::string readSymbol(std::uint64_t symbolAddress) const;

        // hotspot's preallocated symbol table, injected fields name themselves with
        // an index into this instead of a constant pool index
        [[nodiscard]] std::string readVmSymbol(std::uint32_t symbolId) const;

        [[nodiscard]] std::size_t cachedSymbolCount() const noexcept;

    private:
        memory::processMemory memory_;
        const vmStructs *vm_ = nullptr;
        // symbols are shared aggressively across classes, so the same address is read
        // over and over while indexing. copies of a symbolTable share one cache
        mutable std::shared_ptr<std::unordered_map<std::uint64_t, std::string> > cache_;
    };
}
