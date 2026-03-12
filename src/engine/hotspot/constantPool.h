#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <string>

namespace splinter::engine::hotspot {
    class symbolTable;

    enum class constantTag : std::uint16_t {
        invalid = 0,
        utf8 = 1,
        integer = 3,
        floatValue = 4,
        longValue = 5,
        doubleValue = 6,
        klass = 7,
        string = 8,
        fieldRef = 9,
        methodRef = 10,
        interfaceMethodRef = 11,
        nameAndType = 12,
        methodHandle = 15,
        methodType = 16,
        dynamic = 17,
        invokeDynamic = 18,
        unresolvedClass = 100,
        classIndex = 101,
        stringIndex = 102,
        unresolvedClassInError = 103,
        methodHandleInError = 104,
        methodTypeInError = 105,
        dynamicInError = 106
    };

    struct decodedConstantPoolEntry {
        std::int32_t index = 0;
        constantTag tag = constantTag::invalid;
        std::string summary;
    };

    class constantPoolView {
    public:
        constantPoolView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::uint64_t address() const noexcept;

        [[nodiscard]] const memory::processMemory &memory() const noexcept;

        [[nodiscard]] const vmStructs &vm() const noexcept;

        [[nodiscard]] std::optional<std::int32_t> length() const;

        [[nodiscard]] std::optional<std::uint64_t> tagsAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> resolvedKlassesAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> poolHolderAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> cacheAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> operandsAddress() const;

        [[nodiscard]] std::optional<std::uint8_t> tagAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint64_t> rawSlotPointer(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint32_t> rawSlot32(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint64_t> klassAddressAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> classNameIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> unresolvedClassIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> stringIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> methodTypeIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> methodHandleReferenceKindAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> methodHandleReferenceIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> uncachedKlassReferenceIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> uncachedNameAndTypeReferenceIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> bootstrapMethodAttributeIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> bootstrapNameAndTypeReferenceIndexAt(std::int32_t index) const;

        [[nodiscard]] std::optional<std::uint16_t> bootstrapMethodReferenceIndexAt(std::int32_t index) const;

        [[nodiscard]] std::vector<std::uint16_t> bootstrapArgumentIndexesAt(std::int32_t index) const;

        [[nodiscard]] std::string describeBootstrapAt(std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] std::string utf8At(std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] std::string classNameAt(std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] std::string stringAt(std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> nameAndTypeIndexesAt(std::int32_t index) const;

        [[nodiscard]] std::pair<std::string, std::string> nameAndTypeAt(
            std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] decodedConstantPoolEntry decodeAt(std::int32_t index, const symbolTable &symbols) const;

        [[nodiscard]] std::vector<decodedConstantPoolEntry> decodeAll(const symbolTable &symbols,
                                                                      std::size_t limit = 0) const;

    private:
        [[nodiscard]] std::optional<std::uint64_t> resolvedKlassAtIndex(std::uint16_t index) const;

        [[nodiscard]] std::optional<std::uint32_t> operandOffsetAt(std::uint16_t bootstrapMethodAttributeIndex) const;

        [[nodiscard]] std::optional<std::uint16_t> operandAt(std::uint32_t offset) const;

        [[nodiscard]] static std::uint16_t lowShort(std::uint32_t value) noexcept;

        [[nodiscard]] static std::uint16_t highShort(std::uint32_t value) noexcept;

        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}
