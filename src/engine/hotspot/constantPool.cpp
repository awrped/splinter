#include "constantPool.h"

#include "klass.h"
#include "symbolTable.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <format>

namespace splinter::engine::hotspot {
    constantPoolView::constantPoolView(const memory::processMemory &memory, const vmStructs &vm,
                                       std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t constantPoolView::address() const noexcept {
        return address_;
    }

    const memory::processMemory &constantPoolView::memory() const noexcept {
        return *memory_;
    }

    const vmStructs &constantPoolView::vm() const noexcept {
        return *vm_;
    }

    std::optional<std::int32_t> constantPoolView::length() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_length");
        return field != nullptr
                   ? std::optional<std::int32_t>(memory_->read<std::int32_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolView::tagsAddress() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_tags");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolView::resolvedKlassesAddress() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_resolved_klasses");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolView::poolHolderAddress() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_pool_holder");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolView::cacheAddress() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_cache");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolView::operandsAddress() const {
        const vmStructEntry *field = vm_->findField("ConstantPool", "_operands");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint8_t> constantPoolView::tagAt(std::int32_t index) const {
        const auto tags = tagsAddress();
        const vmStructEntry *dataField = vm_->findField("Array<u1>", "_data");
        if (!tags || dataField == nullptr) {
            return std::nullopt;
        }
        return memory_->read<std::uint8_t>(*tags + dataField->offset + static_cast<std::uint64_t>(index));
    }

    std::optional<std::uint64_t> constantPoolView::rawSlotPointer(std::int32_t index) const {
        const auto typeSize = vm_->types().sizeOf("ConstantPool");
        if (!typeSize) {
            return std::nullopt;
        }
        return memory_->read<std::uint64_t>(
            address_ + *typeSize + static_cast<std::uint64_t>(index) * sizeof(std::uint64_t));
    }

    std::optional<std::uint32_t> constantPoolView::rawSlot32(std::int32_t index) const {
        const auto typeSize = vm_->types().sizeOf("ConstantPool");
        if (!typeSize) {
            return std::nullopt;
        }
        return memory_->read<std::uint32_t>(
            address_ + *typeSize + static_cast<std::uint64_t>(index) * sizeof(std::uint64_t));
    }

    std::optional<std::uint64_t> constantPoolView::klassAddressAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        if (tag != constantTag::klass) {
            return std::nullopt;
        }

        const auto resolvedKlasses = resolvedKlassesAddress();
        const auto raw = rawSlot32(index);
        const vmStructEntry *dataField = vm_->findField("Array<Klass*>", "_data");
        if (!resolvedKlasses || !raw || dataField == nullptr) {
            return std::nullopt;
        }

        const std::uint16_t resolvedKlassIndex = lowShort(*raw);
        return resolvedKlassAtIndex(resolvedKlassIndex);
    }

    std::optional<std::uint16_t> constantPoolView::classNameIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw) {
            return std::nullopt;
        }

        if (tag == constantTag::klass || tag == constantTag::unresolvedClass ||
            tag == constantTag::unresolvedClassInError) {
            return highShort(*raw);
        }

        if (tag == constantTag::classIndex) {
            return static_cast<std::uint16_t>(*raw & 0xFFFFu);
        }

        return std::nullopt;
    }

    std::optional<std::uint16_t> constantPoolView::unresolvedClassIndexAt(std::int32_t index) const {
        return classNameIndexAt(index);
    }

    std::optional<std::uint16_t> constantPoolView::stringIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw || tag != constantTag::stringIndex) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(*raw & 0xFFFFu);
    }

    std::optional<std::uint16_t> constantPoolView::methodTypeIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw || (tag != constantTag::methodType && tag != constantTag::methodTypeInError)) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(*raw & 0xFFFFu);
    }

    std::optional<std::uint16_t> constantPoolView::methodHandleReferenceKindAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw || (tag != constantTag::methodHandle && tag != constantTag::methodHandleInError)) {
            return std::nullopt;
        }
        return lowShort(*raw);
    }

    std::optional<std::uint16_t> constantPoolView::methodHandleReferenceIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw || (tag != constantTag::methodHandle && tag != constantTag::methodHandleInError)) {
            return std::nullopt;
        }
        return highShort(*raw);
    }

    std::optional<std::uint16_t> constantPoolView::uncachedKlassReferenceIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw) {
            return std::nullopt;
        }

        if (tag == constantTag::fieldRef || tag == constantTag::methodRef || tag == constantTag::interfaceMethodRef) {
            return lowShort(*raw);
        }

        return std::nullopt;
    }

    std::optional<std::uint16_t> constantPoolView::uncachedNameAndTypeReferenceIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw) {
            return std::nullopt;
        }

        if (tag == constantTag::fieldRef || tag == constantTag::methodRef || tag == constantTag::interfaceMethodRef) {
            return highShort(*raw);
        }

        if (tag == constantTag::dynamic || tag == constantTag::dynamicInError || tag == constantTag::invokeDynamic) {
            return highShort(*raw);
        }

        return std::nullopt;
    }

    std::optional<std::uint16_t> constantPoolView::bootstrapMethodAttributeIndexAt(std::int32_t index) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));
        const auto raw = rawSlot32(index);
        if (!raw) {
            return std::nullopt;
        }

        if (tag == constantTag::dynamic || tag == constantTag::dynamicInError || tag == constantTag::invokeDynamic) {
            return lowShort(*raw);
        }

        return std::nullopt;
    }

    std::optional<std::uint16_t> constantPoolView::bootstrapNameAndTypeReferenceIndexAt(std::int32_t index) const {
        return uncachedNameAndTypeReferenceIndexAt(index);
    }

    std::optional<std::uint16_t> constantPoolView::bootstrapMethodReferenceIndexAt(std::int32_t index) const {
        const auto bootstrapMethodAttributeIndex = bootstrapMethodAttributeIndexAt(index);
        const auto offset = bootstrapMethodAttributeIndex
                                ? operandOffsetAt(*bootstrapMethodAttributeIndex)
                                : std::nullopt;
        return offset ? operandAt(*offset) : std::nullopt;
    }

    std::vector<std::uint16_t> constantPoolView::bootstrapArgumentIndexesAt(std::int32_t index) const {
        std::vector<std::uint16_t> argumentIndexes;
        const auto bootstrapMethodAttributeIndex = bootstrapMethodAttributeIndexAt(index);
        const auto offset = bootstrapMethodAttributeIndex
                                ? operandOffsetAt(*bootstrapMethodAttributeIndex)
                                : std::nullopt;
        if (!offset) {
            return argumentIndexes;
        }

        const auto argc = operandAt(*offset + 1);
        if (!argc) {
            return argumentIndexes;
        }

        argumentIndexes.reserve(*argc);
        for (std::uint32_t argumentIndex = 0; argumentIndex < *argc; ++argumentIndex) {
            const auto value = operandAt(*offset + 2 + argumentIndex);
            if (!value) {
                break;
            }
            argumentIndexes.push_back(*value);
        }
        return argumentIndexes;
    }

    std::string constantPoolView::describeBootstrapAt(std::int32_t index, const symbolTable &symbols) const {
        const auto bootstrapMethodReferenceIndex = bootstrapMethodReferenceIndexAt(index);
        if (!bootstrapMethodReferenceIndex) {
            return {};
        }

        const auto argumentIndexes = bootstrapArgumentIndexesAt(index);
        std::string summary = std::format("bsm=#{}", *bootstrapMethodReferenceIndex);
        const auto bootstrapMethod = decodeAt(*bootstrapMethodReferenceIndex, symbols);
        if (!bootstrapMethod.summary.empty()) {
            summary += std::format(" {}", bootstrapMethod.summary);
        }

        if (!argumentIndexes.empty()) {
            summary += " args=[";
            for (std::size_t argumentIndex = 0; argumentIndex < argumentIndexes.size(); ++argumentIndex) {
                if (argumentIndex != 0) {
                    summary += ", ";
                }
                const auto argument = decodeAt(argumentIndexes[argumentIndex], symbols);
                summary += std::format("#{} {}", argumentIndexes[argumentIndex], argument.summary);
            }
            summary += "]";
        }

        return summary;
    }

    std::string constantPoolView::utf8At(std::int32_t index, const symbolTable &symbols) const {
        const auto slot = rawSlotPointer(index);
        return slot ? symbols.readSymbol(*slot) : std::string{};
    }

    std::string constantPoolView::classNameAt(std::int32_t index, const symbolTable &symbols) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));

        if (tag == constantTag::klass || tag == constantTag::unresolvedClass ||
            tag == constantTag::unresolvedClassInError || tag == constantTag::classIndex) {
            const auto nameIndex = classNameIndexAt(index);
            return nameIndex ? utf8At(*nameIndex, symbols) : std::string{};
        }

        return {};
    }

    std::string constantPoolView::stringAt(std::int32_t index, const symbolTable &symbols) const {
        const auto tag = static_cast<constantTag>(tagAt(index).value_or(0));

        if (tag == constantTag::string) {
            const auto slot = rawSlotPointer(index);
            return slot ? symbols.readSymbol(*slot) : std::string{};
        }

        if (tag == constantTag::stringIndex) {
            const auto indexValue = stringIndexAt(index);
            return indexValue ? utf8At(*indexValue, symbols) : std::string{};
        }

        return {};
    }

    std::pair<std::uint16_t, std::uint16_t> constantPoolView::nameAndTypeIndexesAt(std::int32_t index) const {
        const auto raw = rawSlot32(index);
        if (!raw) {
            return {0, 0};
        }
        return {lowShort(*raw), highShort(*raw)};
    }

    std::pair<std::string, std::string> constantPoolView::nameAndTypeAt(std::int32_t index,
                                                                        const symbolTable &symbols) const {
        const auto [nameIndex, signatureIndex] = nameAndTypeIndexesAt(index);
        return {utf8At(nameIndex, symbols), utf8At(signatureIndex, symbols)};
    }

    decodedConstantPoolEntry constantPoolView::decodeAt(std::int32_t index, const symbolTable &symbols) const {
        decodedConstantPoolEntry entry{};
        entry.index = index;
        entry.tag = static_cast<constantTag>(tagAt(index).value_or(0));

        switch (entry.tag) {
            case constantTag::utf8:
                entry.summary = utf8At(index, symbols);
                break;
            case constantTag::integer: {
                const auto raw = rawSlot32(index).value_or(0);
                const auto value = static_cast<std::int32_t>(raw);
                entry.summary = std::format("{}", value);
                break;
            }
            case constantTag::floatValue: {
                const auto raw = rawSlot32(index).value_or(0);
                const float value = std::bit_cast<float>(raw);
                entry.summary = std::format("{}", value);
                break;
            }
            case constantTag::longValue: {
                const auto raw = rawSlotPointer(index).value_or(0);
                const auto value = static_cast<std::int64_t>(raw);
                entry.summary = std::format("{}", value);
                break;
            }
            case constantTag::doubleValue: {
                const auto raw = rawSlotPointer(index).value_or(0);
                const double value = std::bit_cast<double>(raw);
                entry.summary = std::format("{}", value);
                break;
            }
            case constantTag::klass:
            case constantTag::unresolvedClass:
            case constantTag::unresolvedClassInError:
            case constantTag::classIndex: {
                entry.summary = classNameAt(index, symbols);
                const auto klassAddress = klassAddressAt(index);
                if (klassAddress) {
                    entry.summary += std::format(" @0x{:X}", *klassAddress);
                }
                break;
            }
            case constantTag::string:
            case constantTag::stringIndex:
                entry.summary = stringAt(index, symbols);
                break;
            case constantTag::nameAndType: {
                const auto [name, signature] = nameAndTypeAt(index, symbols);
                entry.summary = std::format("{} {}", name, signature);
                break;
            }
            case constantTag::fieldRef:
            case constantTag::methodRef:
            case constantTag::interfaceMethodRef: {
                const auto classIndex = uncachedKlassReferenceIndexAt(index).value_or(0);
                const auto nameAndTypeIndex = uncachedNameAndTypeReferenceIndexAt(index).value_or(0);
                const auto [name, signature] = nameAndTypeAt(nameAndTypeIndex, symbols);
                entry.summary = std::format("{} {} {}", classNameAt(classIndex, symbols), name, signature);
                break;
            }
            case constantTag::methodType:
            case constantTag::methodTypeInError: {
                const auto signatureIndex = methodTypeIndexAt(index).value_or(0);
                entry.summary = utf8At(signatureIndex, symbols);
                break;
            }
            case constantTag::methodHandle:
            case constantTag::methodHandleInError: {
                const auto referenceKind = methodHandleReferenceKindAt(index).value_or(0);
                const auto referenceIndex = methodHandleReferenceIndexAt(index).value_or(0);
                const auto reference = decodeAt(referenceIndex, symbols);
                entry.summary = std::format("refKind={} refIndex=#{} {}", referenceKind, referenceIndex,
                                            reference.summary);
                break;
            }
            case constantTag::dynamic:
            case constantTag::dynamicInError:
            case constantTag::invokeDynamic: {
                const auto nameAndTypeIndex = bootstrapNameAndTypeReferenceIndexAt(index).value_or(0);
                const auto [name, signature] = nameAndTypeAt(nameAndTypeIndex, symbols);
                entry.summary = std::format("{} {} {}", describeBootstrapAt(index, symbols), name, signature);
                break;
            }
            default:
                entry.summary = std::format("raw=0x{:X}", rawSlotPointer(index).value_or(0));
                break;
        }

        return entry;
    }

    std::vector<decodedConstantPoolEntry>
    constantPoolView::decodeAll(const symbolTable &symbols, std::size_t limit) const {
        std::vector<decodedConstantPoolEntry> entries;
        const auto cpLength = length();
        if (!cpLength) {
            return entries;
        }

        const std::size_t end = limit == 0
                                    ? static_cast<std::size_t>(*cpLength)
                                    : std::min(limit, static_cast<std::size_t>(*cpLength));
        entries.reserve(end > 0 ? end - 1 : 0);

        for (std::size_t index = 1; index < end; ++index) {
            const auto decoded = decodeAt(static_cast<std::int32_t>(index), symbols);
            entries.push_back(decoded);
            if (decoded.tag == constantTag::longValue || decoded.tag == constantTag::doubleValue) {
                ++index;
            }
        }
        return entries;
    }

    std::optional<std::uint64_t> constantPoolView::resolvedKlassAtIndex(std::uint16_t index) const {
        const auto resolvedKlasses = resolvedKlassesAddress();
        const vmStructEntry *dataField = vm_->findField("Array<Klass*>", "_data");
        if (!resolvedKlasses || dataField == nullptr) {
            return std::nullopt;
        }

        return memory_->read<std::uint64_t>(
            *resolvedKlasses + dataField->offset + static_cast<std::uint64_t>(index) * sizeof(std::uint64_t));
    }

    std::optional<std::uint32_t> constantPoolView::operandOffsetAt(std::uint16_t bootstrapMethodAttributeIndex) const {
        const auto operands = operandsAddress();
        const vmStructEntry *dataField = vm_->findField("Array<u2>", "_data");
        if (!operands || dataField == nullptr) {
            return std::nullopt;
        }

        const std::uint64_t baseAddress = *operands + dataField->offset;
        const std::uint64_t slotAddress = baseAddress + static_cast<std::uint64_t>(bootstrapMethodAttributeIndex) * 2ULL
                                          *
                                          sizeof(std::uint16_t);
        const std::uint16_t low = memory_->read<std::uint16_t>(slotAddress);
        const std::uint16_t high = memory_->read<std::uint16_t>(slotAddress + sizeof(std::uint16_t));
        return static_cast<std::uint32_t>(low) | (static_cast<std::uint32_t>(high) << 16U);
    }

    std::optional<std::uint16_t> constantPoolView::operandAt(std::uint32_t offset) const {
        const auto operands = operandsAddress();
        const vmStructEntry *dataField = vm_->findField("Array<u2>", "_data");
        if (!operands || dataField == nullptr) {
            return std::nullopt;
        }

        return memory_->read<std::uint16_t>(
            *operands + dataField->offset + static_cast<std::uint64_t>(offset) * sizeof(std::uint16_t));
    }

    std::uint16_t constantPoolView::lowShort(std::uint32_t value) noexcept {
        return static_cast<std::uint16_t>(value & 0xFFFFu);
    }

    std::uint16_t constantPoolView::highShort(std::uint32_t value) noexcept {
        return static_cast<std::uint16_t>((value >> 16) & 0xFFFFu);
    }
}
