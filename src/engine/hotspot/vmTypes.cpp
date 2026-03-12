#include "vmTypes.h"

namespace splinter::engine::hotspot {
    void vmTypes::clear() noexcept {
        entries_.clear();
    }

    void vmTypes::add(vmTypeInfo entry) {
        entries_.push_back(std::move(entry));
    }

    const std::vector<vmTypeInfo> &vmTypes::entries() const noexcept {
        return entries_;
    }

    const vmTypeInfo *vmTypes::find(std::string_view name) const noexcept {
        for (const auto &entry: entries_) {
            if (entry.name == name) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::optional<std::uint64_t> vmTypes::sizeOf(std::string_view name) const noexcept {
        const vmTypeInfo *entry = find(name);
        if (entry == nullptr) {
            return std::nullopt;
        }
        return entry->size;
    }
}
