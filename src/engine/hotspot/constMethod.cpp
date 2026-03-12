#include "constMethod.h"

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
}