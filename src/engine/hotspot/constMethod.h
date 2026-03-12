#pragma once

#include "vmStructs.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace splinter::engine::hotspot {
    class constantPoolView;
    class symbolTable;

    struct lineNumberEntry {
        std::uint16_t startBci = 0;
        std::uint16_t lineNumber = 0;
    };

    struct localVariableEntry {
        std::uint16_t startBci = 0;
        std::uint16_t length = 0;
        std::uint16_t nameIndex = 0;
        std::uint16_t descriptorIndex = 0;
        std::uint16_t signatureIndex = 0;
        std::uint16_t slot = 0;
        std::string name;
        std::string descriptor;
        std::string genericSignature;
    };

    struct exceptionTableEntry {
        std::uint16_t startPc = 0;
        std::uint16_t endPc = 0;
        std::uint16_t handlerPc = 0;
        std::uint16_t catchTypeIndex = 0;
        std::string catchType;
    };

    struct checkedExceptionEntry {
        std::uint16_t classIndex = 0;
        std::string className;
    };

    struct methodParameterEntry {
        std::uint16_t nameIndex = 0;
        std::uint16_t accessFlags = 0;
        std::string name;
    };

    struct annotationBlobInfo {
        std::uint64_t address = 0;
        std::int32_t length = 0;

        [[nodiscard]] bool present() const noexcept {
            return address != 0;
        }
    };

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

        [[nodiscard]] std::optional<std::uint16_t> genericSignatureIndex() const;

        [[nodiscard]] std::string genericSignature(const constantPoolView &constantPool,
                                                   const symbolTable &symbols) const;

        [[nodiscard]] bool hasLineNumberTable() const;

        [[nodiscard]] bool hasCheckedExceptions() const;

        [[nodiscard]] bool hasLocalVariableTable() const;

        [[nodiscard]] bool hasExceptionTable() const;

        [[nodiscard]] bool hasGenericSignature() const;

        [[nodiscard]] bool hasMethodParameters() const;

        [[nodiscard]] bool hasMethodAnnotations() const;

        [[nodiscard]] bool hasParameterAnnotations() const;

        [[nodiscard]] bool hasTypeAnnotations() const;

        [[nodiscard]] bool hasDefaultAnnotations() const;

        [[nodiscard]] std::optional<std::uint16_t> checkedExceptionsLength() const;

        [[nodiscard]] std::optional<std::uint16_t> localVariableTableLength() const;

        [[nodiscard]] std::optional<std::uint16_t> exceptionTableLength() const;

        [[nodiscard]] std::optional<std::int32_t> methodParametersLength() const;

        [[nodiscard]] annotationBlobInfo methodAnnotations() const;

        [[nodiscard]] annotationBlobInfo parameterAnnotations() const;

        [[nodiscard]] annotationBlobInfo typeAnnotations() const;

        [[nodiscard]] annotationBlobInfo defaultAnnotations() const;

        [[nodiscard]] std::vector<lineNumberEntry> lineNumbers() const;

        [[nodiscard]] std::vector<localVariableEntry> localVariables(const constantPoolView &constantPool,
                                                                     const symbolTable &symbols) const;

        [[nodiscard]] std::vector<exceptionTableEntry> exceptionTable(const constantPoolView &constantPool,
                                                                      const symbolTable &symbols) const;

        [[nodiscard]] std::vector<checkedExceptionEntry> checkedExceptions(const constantPoolView &constantPool,
                                                                           const symbolTable &symbols) const;

        [[nodiscard]] std::vector<methodParameterEntry> methodParameters(const constantPoolView &constantPool,
                                                                         const symbolTable &symbols) const;

        [[nodiscard]] std::vector<std::uint8_t> bytecodes() const;

    private:
        [[nodiscard]] bool flagEnabled(std::string_view constantName) const;

        [[nodiscard]] std::optional<std::int32_t> constMethodSizeWords() const;

        [[nodiscard]] std::optional<std::uint64_t> constMethodEnd() const;

        [[nodiscard]] std::optional<std::uint64_t> codeEndAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> lastU2ElementAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> methodParametersLengthAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> methodParametersStartAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> checkedExceptionsLengthAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> checkedExceptionsStartAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> exceptionTableLengthAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> exceptionTableStartAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> localVariableTableLengthAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> localVariableTableStartAddress() const;

        [[nodiscard]] std::optional<std::int32_t> metaspaceArrayLength(std::uint64_t address) const;

        [[nodiscard]] annotationBlobInfo annotationBlob(std::uint64_t slotFromEnd) const;

        [[nodiscard]] std::uint32_t readUnsigned5(const std::vector<std::byte> &buffer, std::size_t &offset) const;

        [[nodiscard]] std::int32_t readSigned5(const std::vector<std::byte> &buffer, std::size_t &offset) const;

        const memory::processMemory *memory_ = nullptr;
        const vmStructs *vm_ = nullptr;
        std::uint64_t address_ = 0;
    };
}
