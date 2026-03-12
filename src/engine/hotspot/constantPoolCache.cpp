#include "constantPoolCache.h"

namespace splinter::engine::hotspot {
    constantPoolCacheView::constantPoolCacheView(const memory::processMemory &memory,
                                                 const vmStructs &vm,
                                                 std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t constantPoolCacheView::address() const noexcept {
        return address_;
    }

    constantPoolCacheView::layoutMode constantPoolCacheView::mode() const noexcept {
        if (vm_->findField("ConstantPoolCache", "_length") != nullptr) {
            return layoutMode::oldInlineEntries;
        }

        if (vm_->findField("ConstantPoolCache", "_resolved_field_entries") != nullptr ||
            vm_->findField("ConstantPoolCache", "_resolved_method_entries") != nullptr ||
            vm_->findField("ConstantPoolCache", "_resolved_indy_entries") != nullptr) {
            return layoutMode::resolvedEntryArrays;
        }

        return layoutMode::unavailable;
    }

    std::optional<std::uint64_t> constantPoolCacheView::cacheFieldAddress(std::string_view fieldName) const {
        const vmStructEntry *field = vm_->findField("ConstantPoolCache", fieldName);
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolCacheView::resolvedFieldEntriesAddress() const {
        return mode() == layoutMode::resolvedEntryArrays
                   ? cacheFieldAddress("_resolved_field_entries")
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolCacheView::resolvedMethodEntriesAddress() const {
        return mode() == layoutMode::resolvedEntryArrays
                   ? cacheFieldAddress("_resolved_method_entries")
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolCacheView::resolvedIndyEntriesAddress() const {
        return mode() == layoutMode::resolvedEntryArrays
                   ? cacheFieldAddress("_resolved_indy_entries")
                   : std::nullopt;
    }

    std::optional<std::uint64_t> constantPoolCacheView::referenceMapAddress() const {
        return cacheFieldAddress("_reference_map");
    }

    std::optional<std::int32_t> constantPoolCacheView::oldEntryCount() const {
        const vmStructEntry *lengthField = vm_->findField("ConstantPoolCache", "_length");
        return lengthField != nullptr
                   ? std::optional<std::int32_t>(memory_->read<std::int32_t>(address_ + lengthField->offset))
                   : std::nullopt;
    }

    std::optional<std::uint16_t> constantPoolCacheView::fieldConstantPoolIndexAt(std::uint16_t fieldIndex) const {
        if (mode() == layoutMode::oldInlineEntries) {
            return oldEntryConstantPoolIndexAt(fieldIndex);
        }

        const auto entriesAddress = resolvedFieldEntriesAddress();
        const auto entrySize = vm_->types().sizeOf("ResolvedFieldEntry");
        if (!entriesAddress || *entriesAddress == 0 || !entrySize) {
            return std::nullopt;
        }
        return constantPoolIndexAt(*entriesAddress, "Array<ResolvedFieldEntry>", *entrySize, fieldIndex,
                                   "ResolvedFieldEntry");
    }

    std::optional<std::uint16_t> constantPoolCacheView::methodConstantPoolIndexAt(std::uint16_t methodIndex) const {
        if (mode() == layoutMode::oldInlineEntries) {
            return oldEntryConstantPoolIndexAt(methodIndex);
        }

        const auto entriesAddress = resolvedMethodEntriesAddress();
        const auto entrySize = vm_->types().sizeOf("ResolvedMethodEntry");
        if (!entriesAddress || *entriesAddress == 0 || !entrySize) {
            return std::nullopt;
        }
        return constantPoolIndexAt(*entriesAddress, "Array<ResolvedMethodEntry>", *entrySize, methodIndex,
                                   "ResolvedMethodEntry");
    }

    std::optional<std::uint16_t> constantPoolCacheView::indyConstantPoolIndexAt(std::uint16_t indyIndex) const {
        if (mode() == layoutMode::oldInlineEntries) {
            return oldEntryConstantPoolIndexAt(indyIndex);
        }

        const auto entriesAddress = resolvedIndyEntriesAddress();
        const auto entrySize = vm_->types().sizeOf("ResolvedIndyEntry");
        if (!entriesAddress || *entriesAddress == 0 || !entrySize) {
            return std::nullopt;
        }
        return constantPoolIndexAt(*entriesAddress, "Array<ResolvedIndyEntry>", *entrySize, indyIndex,
                                   "ResolvedIndyEntry");
    }

    std::optional<std::uint16_t> constantPoolCacheView::objectConstantPoolIndexAt(std::uint16_t objectIndex) const {
        const auto mapAddress = referenceMapAddress();
        const vmStructEntry *lengthField = vm_->findField("Array<u2>", "_length");
        const vmStructEntry *dataField = vm_->findField("Array<u2>", "_data");
        if (!mapAddress || *mapAddress == 0 || lengthField == nullptr || dataField == nullptr) {
            return std::nullopt;
        }

        const auto length = memory_->read<std::int32_t>(*mapAddress + lengthField->offset);
        if (objectIndex >= static_cast<std::uint16_t>(length)) {
            return std::nullopt;
        }

        return memory_->read<std::uint16_t>(*mapAddress + dataField->offset +
                                            static_cast<std::uint64_t>(objectIndex) * sizeof(std::uint16_t));
    }

    std::uint16_t constantPoolCacheView::decodeInvokedynamicIndex(std::uint32_t encodedIndex) noexcept {
        return static_cast<std::uint16_t>(~encodedIndex);
    }

    std::optional<std::uint64_t> constantPoolCacheView::oldEntryAddress(std::uint16_t entryIndex) const {
        const auto cacheSize = vm_->types().sizeOf("ConstantPoolCache");
        const auto entrySize = vm_->types().sizeOf("ConstantPoolCacheEntry");
        const auto entryCount = oldEntryCount();
        if (!cacheSize || !entrySize || !entryCount || entryIndex >= static_cast<std::uint16_t>(*entryCount)) {
            return std::nullopt;
        }

        return address_ + *cacheSize + static_cast<std::uint64_t>(entryIndex) * *entrySize;
    }

    std::optional<std::uint16_t> constantPoolCacheView::oldEntryConstantPoolIndexAt(std::uint16_t entryIndex) const {
        // jdk22u ConstantPoolCacheEntry layout, the low 16 bits of _indices
        // store the original constant-pool index for the rewritten field/method entry
        // but this can like 100000% change per hotspot so be wary ?!
        constexpr std::uint64_t indicesOffset = 0;
        constexpr std::uint64_t constantPoolIndexMask = 0xFFFF;

        const auto entryAddress = oldEntryAddress(entryIndex);
        if (!entryAddress) {
            return std::nullopt;
        }

        const auto indices = memory_->read<std::uint64_t>(*entryAddress + indicesOffset);
        return static_cast<std::uint16_t>(indices & constantPoolIndexMask);
    }

    std::optional<std::uint16_t> constantPoolCacheView::constantPoolIndexAt(std::uint64_t arrayAddress,
                                                                            std::string_view arrayTypeName,
                                                                            std::size_t entrySize,
                                                                            std::uint16_t entryIndex,
                                                                            std::string_view fieldTypeName) const {
        const vmStructEntry *lengthField = vm_->findField(arrayTypeName, "_length");
        const vmStructEntry *dataField = vm_->findField(arrayTypeName, "_data");
        if (dataField == nullptr) {
            dataField = vm_->findField(arrayTypeName, "_data[0]");
        }
        std::optional<std::uint64_t> cpoolIndexOffset;
        if (fieldTypeName == "ResolvedFieldEntry") {
            const vmStructEntry *field = vm_->findField("ResolvedFieldEntry", "_cpool_index");
            cpoolIndexOffset = field != nullptr ? std::optional<std::uint64_t>(field->offset) : std::nullopt;
        } else if (fieldTypeName == "ResolvedMethodEntry") {
            const vmStructEntry *field = vm_->findField("ResolvedMethodEntry", "_cpool_index");
            cpoolIndexOffset = field != nullptr ? std::optional<std::uint64_t>(field->offset) : std::nullopt;
        } else if (fieldTypeName == "ResolvedIndyEntry") {
            const vmStructEntry *field = vm_->findField("ResolvedIndyEntry", "_cpool_index");
            cpoolIndexOffset = field != nullptr ? std::optional<std::uint64_t>(field->offset) : std::nullopt;
        }

        if (lengthField == nullptr || dataField == nullptr || !cpoolIndexOffset) {
            return std::nullopt;
        }

        const auto length = memory_->read<std::int32_t>(arrayAddress + lengthField->offset);
        if (entryIndex >= static_cast<std::uint16_t>(length)) {
            return std::nullopt;
        }

        const auto entryAddress = arrayAddress + dataField->offset +
                                  static_cast<std::uint64_t>(entryIndex) * static_cast<std::uint64_t>(entrySize);
        return memory_->read<std::uint16_t>(entryAddress + *cpoolIndexOffset);
    }
}
