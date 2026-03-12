#pragma once

#include "constantPool.h"

#include <cstdint>
#include <optional>

namespace splinter::engine::hotspot {
    class constantPoolCacheView {
    public:
        constantPoolCacheView(const memory::processMemory &memory,
                              const vmStructs &vm,
                              std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> referenceMapAddress() const;

        [[nodiscard]] std::optional<std::uint16_t> fieldConstantPoolIndexAt(std::uint16_t fieldIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> methodConstantPoolIndexAt(std::uint16_t methodIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> indyConstantPoolIndexAt(std::uint16_t indyIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> objectConstantPoolIndexAt(std::uint16_t objectIndex) const;

        [[nodiscard]] static std::uint16_t decodeInvokedynamicIndex(std::uint32_t encodedIndex) noexcept;

    private:
        enum class layoutMode {
            unavailable,
            oldInlineEntries,
            resolvedEntryArrays
        };

        [[nodiscard]] layoutMode mode() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> cacheFieldAddress(std::string_view fieldName) const;

        [[nodiscard]] std::optional<std::uint64_t> resolvedFieldEntriesAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> resolvedMethodEntriesAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> resolvedIndyEntriesAddress() const;

        [[nodiscard]] std::optional<std::int32_t> oldEntryCount() const;

        [[nodiscard]] std::optional<std::uint64_t> oldEntryAddress(std::uint16_t entryIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> oldEntryConstantPoolIndexAt(std::uint16_t entryIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> constantPoolIndexAt(std::uint64_t arrayAddress,
                                                                       std::string_view arrayTypeName,
                                                                       std::size_t entrySize,
                                                                       std::uint16_t entryIndex,
                                                                       std::string_view fieldTypeName) const;

        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}