#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace splinter::engine::hotspot {
    class classLoaderDataView {
    public:
        classLoaderDataView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> nextAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> klassesAddress() const;

        [[nodiscard]] std::vector<std::uint64_t> klassAddresses(std::size_t limit = 0) const;

        [[nodiscard]] static std::optional<std::uint64_t> headAddress(const memory::processMemory &memory,
                                                                      const vmStructs &vm);

        [[nodiscard]] static std::vector<std::uint64_t> enumerate(const memory::processMemory &memory,
                                                                  const vmStructs &vm,
                                                                  std::size_t classLoaderLimit = 0,
                                                                  std::size_t klassLimit = 0);

    private:
        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}