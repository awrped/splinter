#pragma once

#include "../memory/processMemory.h"
#include "constantPool.h"
#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace splinter::engine::hotspot {
    class symbolTable;

    struct fieldFlags {
        std::uint32_t raw = 0;

        [[nodiscard]] bool isInitialized() const noexcept;

        [[nodiscard]] bool isInjected() const noexcept;

        [[nodiscard]] bool isGeneric() const noexcept;

        [[nodiscard]] bool isStable() const noexcept;

        [[nodiscard]] bool isContended() const noexcept;
    };

    struct decodedFieldInfo {
        std::uint32_t index = 0;
        std::uint16_t nameIndex = 0;
        std::uint16_t signatureIndex = 0;
        std::uint32_t offset = 0;
        std::uint32_t accessFlags = 0;
        fieldFlags flags{};
        std::uint16_t initializerIndex = 0;
        std::uint16_t genericSignatureIndex = 0;
        std::uint16_t contentionGroup = 0;

        std::string name;
        std::string signature;
        std::string genericSignature;
    };

    class fieldInfoView {
    public:
        fieldInfoView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t streamAddress) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] std::optional<std::uint32_t> javaFieldCount() const;

        [[nodiscard]] std::optional<std::uint32_t> injectedFieldCount() const;

        [[nodiscard]] std::optional<std::uint32_t> totalFieldCount() const;

        [[nodiscard]] std::vector<decodedFieldInfo> decodeAll(const constantPoolView &constantPool,
                                                              const symbolTable &symbols,
                                                              std::size_t limit = 0) const;

    private:
        class unsigned5Reader {
        public:
            explicit unsigned5Reader(const std::vector<std::byte> &buffer) noexcept;

            [[nodiscard]] bool hasNext() const noexcept;

            [[nodiscard]] std::size_t position() const noexcept;

            [[nodiscard]] std::uint32_t nextUint();

        private:
            const std::vector<std::byte> *buffer_ = nullptr;
            std::size_t position_ = 0;
        };

        [[nodiscard]] std::vector<std::byte> readStream() const;

        [[nodiscard]] decodedFieldInfo decodeOne(unsigned5Reader &reader,
                                                 std::uint32_t index,
                                                 const constantPoolView &constantPool,
                                                 const symbolTable &symbols) const;

        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}
