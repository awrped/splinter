#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <string>

namespace splinter::engine::hotspot {
    class symbolTable;

    class klassView {
    public:
        klassView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::int32_t> layoutHelper() const;

        [[nodiscard]] bool isInstanceKlass() const;

        [[nodiscard]] std::optional<std::uint64_t> nameAddress() const;

        [[nodiscard]] std::string name(const symbolTable &symbols) const;

    protected:
        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}
