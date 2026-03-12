#include "constMethod.h"

#include "constantPool.h"
#include "symbolTable.h"

#include <bit>
#include <cstddef>
#include <stdexcept>

namespace splinter::engine::hotspot {
    constMethodView::constMethodView(const memory::processMemory &memory, const vmStructs &vm,
                                     std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t constMethodView::address() const noexcept {
        return address_;
    }

    std::optional<std::uint64_t> constMethodView::constantsAddress() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_constants");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::codeSize() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_code_size");
        return field != nullptr
                   ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::nameIndex() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_name_index");
        return field != nullptr
                   ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::signatureIndex() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_signature_index");
        return field != nullptr
                   ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::maxStack() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_max_stack");
        return field != nullptr
                   ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::maxLocals() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_max_locals");
        return field != nullptr
                   ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint32_t> constMethodView::flags() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_flags._flags");
        return field != nullptr
                   ? std::optional<std::uint32_t>(memory_->read<std::uint32_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::genericSignatureIndex() const {
        if (!hasGenericSignature()) {
            return std::nullopt;
        }

        const auto address = lastU2ElementAddress();
        return address ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(*address)) : std::nullopt;
    }

    std::string constMethodView::genericSignature(const constantPoolView &constantPool,
                                                  const symbolTable &symbols) const {
        const auto index = genericSignatureIndex();
        return index ? constantPool.utf8At(*index, symbols) : std::string();
    }

    bool constMethodView::hasLineNumberTable() const {
        return flagEnabled("ConstMethodFlags::_misc_has_linenumber_table");
    }

    bool constMethodView::hasCheckedExceptions() const {
        return flagEnabled("ConstMethodFlags::_misc_has_checked_exceptions");
    }

    bool constMethodView::hasLocalVariableTable() const {
        return flagEnabled("ConstMethodFlags::_misc_has_localvariable_table");
    }

    bool constMethodView::hasExceptionTable() const {
        return flagEnabled("ConstMethodFlags::_misc_has_exception_table");
    }

    bool constMethodView::hasGenericSignature() const {
        return flagEnabled("ConstMethodFlags::_misc_has_generic_signature");
    }

    bool constMethodView::hasMethodParameters() const {
        return flagEnabled("ConstMethodFlags::_misc_has_method_parameters");
    }

    bool constMethodView::hasMethodAnnotations() const {
        return flagEnabled("ConstMethodFlags::_misc_has_method_annotations");
    }

    bool constMethodView::hasParameterAnnotations() const {
        return flagEnabled("ConstMethodFlags::_misc_has_parameter_annotations");
    }

    bool constMethodView::hasTypeAnnotations() const {
        return flagEnabled("ConstMethodFlags::_misc_has_type_annotations");
    }

    bool constMethodView::hasDefaultAnnotations() const {
        return flagEnabled("ConstMethodFlags::_misc_has_default_annotations");
    }

    std::optional<std::uint16_t> constMethodView::checkedExceptionsLength() const {
        const auto address = checkedExceptionsLengthAddress();
        return address ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(*address)) : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::localVariableTableLength() const {
        const auto address = localVariableTableLengthAddress();
        return address ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(*address)) : std::nullopt;
    }

    std::optional<std::uint16_t> constMethodView::exceptionTableLength() const {
        const auto address = exceptionTableLengthAddress();
        return address ? std::optional<std::uint16_t>(memory_->read<std::uint16_t>(*address)) : std::nullopt;
    }

    std::optional<std::int32_t> constMethodView::methodParametersLength() const {
        const auto address = methodParametersLengthAddress();
        return address ? std::optional<std::int32_t>(memory_->read<std::uint16_t>(*address)) : std::nullopt;
    }

    annotationBlobInfo constMethodView::methodAnnotations() const {
        if (!hasMethodAnnotations()) {
            return {};
        }
        return annotationBlob(1);
    }

    annotationBlobInfo constMethodView::parameterAnnotations() const {
        if (!hasParameterAnnotations()) {
            return {};
        }
        return annotationBlob(hasMethodAnnotations() ? 2 : 1);
    }

    annotationBlobInfo constMethodView::typeAnnotations() const {
        if (!hasTypeAnnotations()) {
            return {};
        }
        return annotationBlob(1 + (hasMethodAnnotations() ? 1 : 0) + (hasParameterAnnotations() ? 1 : 0));
    }

    annotationBlobInfo constMethodView::defaultAnnotations() const {
        if (!hasDefaultAnnotations()) {
            return {};
        }
        return annotationBlob(1 + (hasMethodAnnotations() ? 1 : 0) + (hasParameterAnnotations() ? 1 : 0) +
                              (hasTypeAnnotations() ? 1 : 0));
    }

    std::vector<lineNumberEntry> constMethodView::lineNumbers() const {
        std::vector<lineNumberEntry> entries;
        if (!hasLineNumberTable()) {
            return entries;
        }

        const auto lineTableAddress = codeEndAddress();
        const auto endAddress = constMethodEnd();
        if (!lineTableAddress || !endAddress || *lineTableAddress >= *endAddress) {
            return entries;
        }

        const auto buffer = memory_->readBuffer(*lineTableAddress,
                                                static_cast<std::size_t>(*endAddress - *lineTableAddress));
        std::size_t offset = 0;
        std::int32_t bci = 0;
        std::int32_t line = 0;
        while (offset < buffer.size()) {
            const auto next = std::to_integer<std::uint8_t>(buffer[offset++]);
            if (next == 0) {
                break;
            }

            if (next == 0xFF) {
                bci += readSigned5(buffer, offset);
                line += readSigned5(buffer, offset);
            } else {
                bci += static_cast<std::int32_t>(next >> 3);
                line += static_cast<std::int32_t>(next & 0x7);
            }

            entries.push_back(lineNumberEntry{
                static_cast<std::uint16_t>(bci),
                static_cast<std::uint16_t>(line)
            });
        }

        return entries;
    }

    std::vector<localVariableEntry> constMethodView::localVariables(const constantPoolView &constantPool,
                                                                    const symbolTable &symbols) const {
        std::vector<localVariableEntry> entries;
        const auto count = localVariableTableLength();
        const auto start = localVariableTableStartAddress();
        if (!count || !start || *count == 0) {
            return entries;
        }

        entries.reserve(*count);
        for (std::uint16_t index = 0; index < *count; ++index) {
            const auto entryAddress = *start + static_cast<std::uint64_t>(index) * 6 * sizeof(std::uint16_t);
            localVariableEntry entry{};
            entry.startBci = memory_->read<std::uint16_t>(entryAddress);
            entry.length = memory_->read<std::uint16_t>(entryAddress + 2);
            entry.nameIndex = memory_->read<std::uint16_t>(entryAddress + 4);
            entry.descriptorIndex = memory_->read<std::uint16_t>(entryAddress + 6);
            entry.signatureIndex = memory_->read<std::uint16_t>(entryAddress + 8);
            entry.slot = memory_->read<std::uint16_t>(entryAddress + 10);
            entry.name = entry.nameIndex != 0 ? constantPool.utf8At(entry.nameIndex, symbols) : std::string();
            entry.descriptor =
                    entry.descriptorIndex != 0 ? constantPool.utf8At(entry.descriptorIndex, symbols) : std::string();
            entry.genericSignature =
                    entry.signatureIndex != 0 ? constantPool.utf8At(entry.signatureIndex, symbols) : std::string();
            entries.push_back(std::move(entry));
        }

        return entries;
    }

    std::vector<exceptionTableEntry> constMethodView::exceptionTable(const constantPoolView &constantPool,
                                                                     const symbolTable &symbols) const {
        std::vector<exceptionTableEntry> entries;
        const auto count = exceptionTableLength();
        const auto start = exceptionTableStartAddress();
        if (!count || !start || *count == 0) {
            return entries;
        }

        entries.reserve(*count);
        for (std::uint16_t index = 0; index < *count; ++index) {
            const auto entryAddress = *start + static_cast<std::uint64_t>(index) * 4 * sizeof(std::uint16_t);
            exceptionTableEntry entry{};
            entry.startPc = memory_->read<std::uint16_t>(entryAddress);
            entry.endPc = memory_->read<std::uint16_t>(entryAddress + 2);
            entry.handlerPc = memory_->read<std::uint16_t>(entryAddress + 4);
            entry.catchTypeIndex = memory_->read<std::uint16_t>(entryAddress + 6);
            entry.catchType = entry.catchTypeIndex != 0
                                  ? constantPool.classNameAt(entry.catchTypeIndex, symbols)
                                  : std::string("any");
            entries.push_back(std::move(entry));
        }

        return entries;
    }

    std::vector<checkedExceptionEntry> constMethodView::checkedExceptions(const constantPoolView &constantPool,
                                                                          const symbolTable &symbols) const {
        std::vector<checkedExceptionEntry> entries;
        const auto count = checkedExceptionsLength();
        const auto start = checkedExceptionsStartAddress();
        if (!count || !start || *count == 0) {
            return entries;
        }

        entries.reserve(*count);
        for (std::uint16_t index = 0; index < *count; ++index) {
            checkedExceptionEntry entry{};
            entry.classIndex = memory_->read<std::uint16_t>(
                *start + static_cast<std::uint64_t>(index) * sizeof(std::uint16_t));
            entry.className = entry.classIndex != 0
                                  ? constantPool.classNameAt(entry.classIndex, symbols)
                                  : std::string();
            entries.push_back(std::move(entry));
        }

        return entries;
    }

    std::vector<methodParameterEntry> constMethodView::methodParameters(const constantPoolView &constantPool,
                                                                        const symbolTable &symbols) const {
        std::vector<methodParameterEntry> entries;
        const auto count = methodParametersLength();
        const auto start = methodParametersStartAddress();
        if (!count || !start || *count < 0) {
            return entries;
        }

        entries.reserve(static_cast<std::size_t>(*count));
        for (std::int32_t index = 0; index < *count; ++index) {
            const auto entryAddress = *start + static_cast<std::uint64_t>(index) * 2 * sizeof(std::uint16_t);
            methodParameterEntry entry{};
            entry.nameIndex = memory_->read<std::uint16_t>(entryAddress);
            entry.accessFlags = memory_->read<std::uint16_t>(entryAddress + 2);
            entry.name = entry.nameIndex != 0 ? constantPool.utf8At(entry.nameIndex, symbols) : std::string();
            entries.push_back(std::move(entry));
        }

        return entries;
    }

    std::vector<std::uint8_t> constMethodView::bytecodes() const {
        std::vector<std::uint8_t> result;
        const auto typeSize = vm_->types().sizeOf("ConstMethod");
        const auto size = codeSize();
        if (!typeSize || !size) {
            return result;
        }

        const auto buffer = memory_->readBuffer(address_ + *typeSize, *size);
        result.reserve(buffer.size());
        for (const auto byte: buffer) {
            result.push_back(static_cast<std::uint8_t>(byte));
        }
        return result;
    }

    bool constMethodView::flagEnabled(std::string_view constantName) const {
        const auto mask = vm_->constants().findInt(constantName);
        const auto value = flags();
        return mask && value && ((*value & static_cast<std::uint32_t>(*mask)) != 0);
    }

    std::optional<std::int32_t> constMethodView::constMethodSizeWords() const {
        const vmStructEntry *field = vm_->findField("ConstMethod", "_constMethod_size");
        if (field == nullptr) {
            return std::nullopt;
        }

        const auto sizeInWords = memory_->read<std::int32_t>(address_ + field->offset);
        return sizeInWords > 0 ? std::optional<std::int32_t>(sizeInWords) : std::nullopt;
    }

    std::optional<std::uint64_t> constMethodView::constMethodEnd() const {
        const auto sizeInWords = constMethodSizeWords();
        return sizeInWords
                   ? std::optional<std::uint64_t>(
                       address_ + static_cast<std::uint64_t>(*sizeInWords) * sizeof(std::uint64_t))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constMethodView::codeEndAddress() const {
        const auto typeSize = vm_->types().sizeOf("ConstMethod");
        const auto size = codeSize();
        if (!typeSize || !size) {
            return std::nullopt;
        }

        return address_ + *typeSize + *size;
    }

    std::optional<std::uint64_t> constMethodView::lastU2ElementAddress() const {
        const auto endAddress = constMethodEnd();
        if (!endAddress) {
            return std::nullopt;
        }

        std::uint64_t pointerCount = 0;
        pointerCount += hasMethodAnnotations() ? 1 : 0;
        pointerCount += hasParameterAnnotations() ? 1 : 0;
        pointerCount += hasTypeAnnotations() ? 1 : 0;
        pointerCount += hasDefaultAnnotations() ? 1 : 0;
        return *endAddress - pointerCount * sizeof(std::uint64_t) - sizeof(std::uint16_t);
    }

    std::optional<std::uint64_t> constMethodView::methodParametersLengthAddress() const {
        if (!hasMethodParameters()) {
            return std::nullopt;
        }

        const auto tail = lastU2ElementAddress();
        if (!tail) {
            return std::nullopt;
        }

        return hasGenericSignature() ? *tail - sizeof(std::uint16_t) : *tail;
    }

    std::optional<std::uint64_t> constMethodView::methodParametersStartAddress() const {
        const auto lengthAddress = methodParametersLengthAddress();
        if (!lengthAddress) {
            return std::nullopt;
        }

        const auto count = memory_->read<std::uint16_t>(*lengthAddress);
        return *lengthAddress - static_cast<std::uint64_t>(count) * 2 * sizeof(std::uint16_t);
    }

    std::optional<std::uint64_t> constMethodView::checkedExceptionsLengthAddress() const {
        if (!hasCheckedExceptions()) {
            return std::nullopt;
        }

        if (hasMethodParameters()) {
            const auto start = methodParametersStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        const auto tail = lastU2ElementAddress();
        if (!tail) {
            return std::nullopt;
        }
        return hasGenericSignature() ? *tail - sizeof(std::uint16_t) : *tail;
    }

    std::optional<std::uint64_t> constMethodView::checkedExceptionsStartAddress() const {
        const auto lengthAddress = checkedExceptionsLengthAddress();
        if (!lengthAddress) {
            return std::nullopt;
        }

        const auto count = memory_->read<std::uint16_t>(*lengthAddress);
        return *lengthAddress - static_cast<std::uint64_t>(count) * sizeof(std::uint16_t);
    }

    std::optional<std::uint64_t> constMethodView::exceptionTableLengthAddress() const {
        if (!hasExceptionTable()) {
            return std::nullopt;
        }

        if (hasCheckedExceptions()) {
            const auto start = checkedExceptionsStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        if (hasMethodParameters()) {
            const auto start = methodParametersStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        const auto tail = lastU2ElementAddress();
        if (!tail) {
            return std::nullopt;
        }
        return hasGenericSignature() ? *tail - sizeof(std::uint16_t) : *tail;
    }

    std::optional<std::uint64_t> constMethodView::exceptionTableStartAddress() const {
        const auto lengthAddress = exceptionTableLengthAddress();
        if (!lengthAddress) {
            return std::nullopt;
        }

        const auto count = memory_->read<std::uint16_t>(*lengthAddress);
        return *lengthAddress - static_cast<std::uint64_t>(count) * 4 * sizeof(std::uint16_t);
    }

    std::optional<std::uint64_t> constMethodView::localVariableTableLengthAddress() const {
        if (!hasLocalVariableTable()) {
            return std::nullopt;
        }

        if (hasExceptionTable()) {
            const auto start = exceptionTableStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        if (hasCheckedExceptions()) {
            const auto start = checkedExceptionsStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        if (hasMethodParameters()) {
            const auto start = methodParametersStartAddress();
            return start ? std::optional<std::uint64_t>(*start - sizeof(std::uint16_t)) : std::nullopt;
        }

        const auto tail = lastU2ElementAddress();
        if (!tail) {
            return std::nullopt;
        }
        return hasGenericSignature() ? *tail - sizeof(std::uint16_t) : *tail;
    }

    std::optional<std::uint64_t> constMethodView::localVariableTableStartAddress() const {
        const auto lengthAddress = localVariableTableLengthAddress();
        if (!lengthAddress) {
            return std::nullopt;
        }

        const auto count = memory_->read<std::uint16_t>(*lengthAddress);
        return *lengthAddress - static_cast<std::uint64_t>(count) * 6 * sizeof(std::uint16_t);
    }

    std::optional<std::int32_t> constMethodView::metaspaceArrayLength(std::uint64_t address) const {
        const vmStructEntry *lengthField = vm_->findField("Array<int>", "_length");
        if (address == 0 || lengthField == nullptr) {
            return std::nullopt;
        }

        return memory_->read<std::int32_t>(address + lengthField->offset);
    }

    annotationBlobInfo constMethodView::annotationBlob(std::uint64_t slotFromEnd) const {
        annotationBlobInfo info{};
        const auto endAddress = constMethodEnd();
        if (!endAddress || slotFromEnd == 0) {
            return info;
        }

        const auto pointerAddress = *endAddress - slotFromEnd * sizeof(std::uint64_t);
        info.address = memory_->read<std::uint64_t>(pointerAddress);
        info.length = metaspaceArrayLength(info.address).value_or(0);
        return info;
    }

    std::uint32_t constMethodView::readUnsigned5(const std::vector<std::byte> &buffer, std::size_t &offset) const {
        if (offset >= buffer.size()) {
            throw std::runtime_error("Compressed stream overflow");
        }

        const std::size_t start = offset;
        const std::uint32_t first = std::to_integer<std::uint8_t>(buffer[offset]);
        if (first < 1) {
            throw std::runtime_error("Compressed stream used an excluded byte");
        }

        std::uint32_t sum = first - 1;
        if (sum < 191) {
            ++offset;
            return sum;
        }

        std::uint32_t shift = 6;
        for (std::size_t index = 1; index < 5; ++index) {
            if (start + index >= buffer.size()) {
                throw std::runtime_error("Compressed stream ended mid-value");
            }

            const std::uint32_t value = std::to_integer<std::uint8_t>(buffer[start + index]);
            if (value < 1) {
                throw std::runtime_error("Compressed stream used an excluded byte");
            }

            sum += (value - 1) << shift;
            if (value < 192 || index == 4) {
                offset = start + index + 1;
                return sum;
            }

            shift += 6;
        }

        throw std::runtime_error("Invalid compressed value");
    }

    std::int32_t constMethodView::readSigned5(const std::vector<std::byte> &buffer, std::size_t &offset) const {
        const auto value = readUnsigned5(buffer, offset);
        return static_cast<std::int32_t>((value >> 1) ^ -(static_cast<std::int32_t>(value & 1)));
    }
}
