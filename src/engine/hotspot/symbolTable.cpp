#include "symbolTable.h"

namespace splinter::engine::hotspot {
    symbolTable::symbolTable(const memory::processMemory &memory, const vmStructs &vm) noexcept : memory_(memory),
        vm_(&vm) {
    }

    std::string symbolTable::readSymbol(std::uint64_t symbolAddress) const {
        if (symbolAddress == 0) {
            return {};
        }

        const vmStructEntry *lengthField = vm_->findField("Symbol", "_length");
        const vmStructEntry *bodyField = vm_->findField("Symbol", "_body");
        if (lengthField == nullptr || bodyField == nullptr) {
            return {};
        }

        const std::uint16_t length = memory_.read<std::uint16_t>(symbolAddress + lengthField->offset);
        const auto buffer = memory_.readBuffer(symbolAddress + bodyField->offset, length);
        return std::string(reinterpret_cast<const char *>(buffer.data()),
                           reinterpret_cast<const char *>(buffer.data()) + buffer.size());
    }
}