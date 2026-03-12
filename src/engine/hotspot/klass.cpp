#include "arrayKlass.h"
#include "klass.h"

#include "symbolTable.h"

namespace splinter::engine::hotspot {
    klassView::klassView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t klassView::address() const noexcept {
        return address_;
    }

    std::optional<std::int32_t> klassView::layoutHelper() const {
        const vmStructEntry *field = vm_->findField("Klass", "_layout_helper");
        if (field == nullptr || field->isStatic) {
            return std::nullopt;
        }
        return memory_->read<std::int32_t>(address_ + field->offset);
    }

    bool klassView::isInstanceKlass() const {
        const auto helper = layoutHelper();
        return helper.has_value() && *helper > 0;
    }

    std::optional<std::uint64_t> klassView::nameAddress() const {
        const vmStructEntry *field = vm_->findField("Klass", "_name");
        if (field == nullptr || field->isStatic) {
            return std::nullopt;
        }
        return memory_->read<std::uint64_t>(address_ + field->offset);
    }

    std::string klassView::name(const symbolTable &symbols) const {
        const auto symbolAddress = nameAddress();
        return symbolAddress ? symbols.readSymbol(*symbolAddress) : std::string{};
    }

    arrayKlassView::arrayKlassView(const memory::processMemory &memory, const vmStructs &vm,
                                   std::uint64_t address) noexcept
        : klassView(memory, vm, address) {
    }
}
