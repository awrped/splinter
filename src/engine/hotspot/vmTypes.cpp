#include "vmTypes.h"

namespace splinter::engine::hotspot {
    void vmTypes::clear() noexcept {
        entries_.clear();
        lookup_.clear();
    }

    void vmTypes::add(vmTypeInfo entry) {
        // duplicate names keep the first entry, matching the order the table exports
        lookup_.try_emplace(entry.name, entries_.size());
        entries_.push_back(std::move(entry));
    }

    const std::vector<vmTypeInfo> &vmTypes::entries() const noexcept {
        return entries_;
    }

    const vmTypeInfo *vmTypes::find(std::string_view name) const noexcept {
        const auto found = lookup_.find(name);
        return found != lookup_.end() ? &entries_[found->second] : nullptr;
    }

    std::optional<std::uint64_t> vmTypes::sizeOf(std::string_view name) const noexcept {
        const vmTypeInfo *entry = find(name);
        if (entry == nullptr) {
            return std::nullopt;
        }
        return entry->size;
    }
}
