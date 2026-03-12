#include "method.h"

#include "constMethod.h"
#include "constantPool.h"
#include "symbolTable.h"

namespace splinter::engine::hotspot {
    methodView::methodView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t methodView::address() const noexcept {
        return address_;
    }

    std::optional<std::uint64_t> methodView::constMethodAddress() const {
        const vmStructEntry *field = vm_->findField("Method", "_constMethod");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> methodView::methodDataAddress() const {
        const vmStructEntry *field = vm_->findField("Method", "_method_data");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> methodView::methodCountersAddress() const {
        const vmStructEntry *field = vm_->findField("Method", "_method_counters");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint32_t> methodView::accessFlags() const {
        const vmStructEntry *field = vm_->findField("Method", "_access_flags._flags");
        return field != nullptr
                   ? std::optional<std::uint32_t>(memory_->read<std::uint32_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::int32_t> methodView::vtableIndex() const {
        const vmStructEntry *field = vm_->findField("Method", "_vtable_index");
        return field != nullptr
                   ? std::optional<std::int32_t>(memory_->read<std::int32_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::string methodView::name(const constantPoolView &constantPool, const symbolTable &symbols) const {
        const auto constMethodAddressValue = constMethodAddress();
        if (!constMethodAddressValue) {
            return {};
        }

        const constMethodView constMethod(*memory_, *vm_, *constMethodAddressValue);
        const auto nameIndex = constMethod.nameIndex();
        return nameIndex ? constantPool.utf8At(*nameIndex, symbols) : std::string{};
    }

    std::string methodView::signature(const constantPoolView &constantPool, const symbolTable &symbols) const {
        const auto constMethodAddressValue = constMethodAddress();
        if (!constMethodAddressValue) {
            return {};
        }

        const constMethodView constMethod(*memory_, *vm_, *constMethodAddressValue);
        const auto signatureIndex = constMethod.signatureIndex();
        return signatureIndex ? constantPool.utf8At(*signatureIndex, symbols) : std::string{};
    }
}
