#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <string>

namespace splinter::engine::hotspot {
    class constantPoolView;
    class symbolTable;

    class methodView {
    public:
        methodView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> constMethodAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> methodDataAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> methodCountersAddress() const;

        [[nodiscard]] std::optional<std::uint32_t> accessFlags() const;

        [[nodiscard]] std::optional<std::int32_t> vtableIndex() const;

        [[nodiscard]] std::string name(const constantPoolView &constantPool, const symbolTable &symbols) const;

        [[nodiscard]] std::string signature(const constantPoolView &constantPool, const symbolTable &symbols) const;

    private:
        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}
