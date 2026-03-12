#include "instanceKlass.h"

namespace splinter::engine::hotspot {
    instanceKlassView::instanceKlassView(const memory::processMemory &memory, const vmStructs &vm,
                                         std::uint64_t address) noexcept
        : klassView(memory, vm, address) {
    }

    std::optional<std::uint64_t> instanceKlassView::constantsAddress() const {
        const vmStructEntry *field = vm_->findField("InstanceKlass", "_constants");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> instanceKlassView::methodsArrayAddress() const {
        const vmStructEntry *field = vm_->findField("InstanceKlass", "_methods");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> instanceKlassView::fieldInfoStreamAddress() const {
        const vmStructEntry *field = vm_->findField("InstanceKlass", "_fieldinfo_stream");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::int32_t> instanceKlassView::javaFieldCount() const {
        const auto streamAddress = fieldInfoStreamAddress();
        if (!streamAddress || *streamAddress == 0) {
            return std::nullopt;
        }

        const fieldInfoView fields(*memory_, *vm_, *streamAddress);
        const auto count = fields.javaFieldCount();
        return count ? std::optional<std::int32_t>(static_cast<std::int32_t>(*count)) : std::nullopt;
    }

    std::optional<std::int32_t> instanceKlassView::totalFieldCount() const {
        const auto streamAddress = fieldInfoStreamAddress();
        if (!streamAddress || *streamAddress == 0) {
            return std::nullopt;
        }

        const fieldInfoView fields(*memory_, *vm_, *streamAddress);
        const auto count = fields.totalFieldCount();
        return count ? std::optional<std::int32_t>(static_cast<std::int32_t>(*count)) : std::nullopt;
    }

    std::vector<std::uint64_t> instanceKlassView::methodAddresses() const {
        std::vector<std::uint64_t> result;
        const auto arrayAddress = methodsArrayAddress();
        const vmStructEntry *lengthField = vm_->findField("Array<int>", "_length");
        const vmStructEntry *dataField = vm_->findField("Array<Method*>", "_data");
        if (!arrayAddress || lengthField == nullptr || dataField == nullptr) {
            return result;
        }

        const std::int32_t length = memory_->read<std::int32_t>(*arrayAddress + lengthField->offset);
        result.reserve(length);
        for (std::int32_t index = 0; index < length; ++index) {
            result.push_back(memory_->read<std::uint64_t>(
                *arrayAddress + dataField->offset + static_cast<std::uint64_t>(index) * sizeof(std::uint64_t)));
        }
        return result;
    }

    std::vector<decodedFieldInfo> instanceKlassView::fields(const symbolTable &symbols, std::size_t limit) const {
        const auto streamAddress = fieldInfoStreamAddress();
        const auto constants = constantsAddress();
        if (!streamAddress || !constants || *streamAddress == 0 || *constants == 0) {
            return {};
        }

        const constantPoolView constantPool(*memory_, *vm_, *constants);
        const fieldInfoView fieldInfo(*memory_, *vm_, *streamAddress);
        return fieldInfo.decodeAll(constantPool, symbols, limit);
    }
}
