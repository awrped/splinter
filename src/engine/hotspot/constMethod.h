#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace splinter::engine::hotspot {
    class constMethodView {
    public:
        constMethodView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> constantsAddress() const;

        [[nodiscard]] std::optional<std::uint16_t> codeSize() const;

        [[nodiscard]] std::optional<std::uint16_t> nameIndex() const;

        [[nodiscard]] std::optional<std::uint16_t> signatureIndex() const;

        [[nodiscard]] std::optional<std::uint16_t> maxStack() const;

        [[nodiscard]] std::optional<std::uint16_t> maxLocals() const;

        [[nodiscard]] std::optional<std::uint32_t> flags() const;

        [[nodiscard]] std::vector<std::uint8_t> bytecodes() const;

    private:
        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}