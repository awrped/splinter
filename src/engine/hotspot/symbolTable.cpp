#include "symbolTable.h"

namespace splinter::engine::hotspot {
    symbolTable::symbolTable(const memory::processMemory &memory, const vmStructs &vm) noexcept : memory_(memory),
        vm_(&vm) {
    }

    std::string symbolTable::readSymbol(std::uint64_t symbolAddress) const {
        if (symbolAddress == 0) {
            return {};
        }

        if (cache_) {
            if (const auto cached = cache_->find(symbolAddress); cached != cache_->end()) {
                return cached->second;
            }
        }

        const vmStructEntry *lengthField = vm_->findField("Symbol", "_length");
        const vmStructEntry *bodyField = vm_->findField("Symbol", "_body");
        if (lengthField == nullptr || bodyField == nullptr) {
            return {};
        }

        const std::uint16_t length = memory_.read<std::uint16_t>(symbolAddress + lengthField->offset);
        const auto buffer = memory_.readBuffer(symbolAddress + bodyField->offset, length);
        std::string symbol(reinterpret_cast<const char *>(buffer.data()),
                           reinterpret_cast<const char *>(buffer.data()) + buffer.size());

        if (!cache_) {
            // built lazily so a symbolTable that is never used stays free
            cache_ = std::make_shared<std::unordered_map<std::uint64_t, std::string> >();
        }
        cache_->emplace(symbolAddress, symbol);
        return symbol;
    }

    std::string symbolTable::readVmSymbol(std::uint32_t symbolId) const {
        const vmStructEntry *table = vm_->findField("Symbol", "_vm_symbols[0]");
        const auto firstSid = vm_->constants().findInt("vmSymbols::FIRST_SID");
        const auto sidLimit = vm_->constants().findInt("vmSymbols::SID_LIMIT");
        if (table == nullptr || !table->isStatic || !firstSid || !sidLimit) {
            return {};
        }

        const auto id = static_cast<std::int32_t>(symbolId);
        if (id < *firstSid || id >= *sidLimit) {
            return {};
        }

        const auto symbolAddress = memory_.read<std::uint64_t>(
            table->address + static_cast<std::uint64_t>(id) * sizeof(std::uint64_t));
        return readSymbol(symbolAddress);
    }

    std::size_t symbolTable::cachedSymbolCount() const noexcept {
        return cache_ ? cache_->size() : 0;
    }
}
