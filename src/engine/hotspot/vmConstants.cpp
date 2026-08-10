#include "vmConstants.h"

namespace splinter::engine::hotspot {
    void vmConstants::clear() noexcept {
        intEntries_.clear();
        longEntries_.clear();
        intLookup_.clear();
        longLookup_.clear();
    }

    void vmConstants::add(vmIntConstant entry) {
        intLookup_.try_emplace(entry.name, intEntries_.size());
        intEntries_.push_back(std::move(entry));
    }

    void vmConstants::add(vmLongConstant entry) {
        longLookup_.try_emplace(entry.name, longEntries_.size());
        longEntries_.push_back(std::move(entry));
    }

    const std::vector<vmIntConstant> &vmConstants::intEntries() const noexcept {
        return intEntries_;
    }

    const std::vector<vmLongConstant> &vmConstants::longEntries() const noexcept {
        return longEntries_;
    }

    std::optional<std::int32_t> vmConstants::findInt(std::string_view name) const noexcept {
        const auto found = intLookup_.find(name);
        return found != intLookup_.end()
                   ? std::optional<std::int32_t>(intEntries_[found->second].value)
                   : std::nullopt;
    }

    std::optional<std::uint64_t> vmConstants::findLong(std::string_view name) const noexcept {
        const auto found = longLookup_.find(name);
        return found != longLookup_.end()
                   ? std::optional<std::uint64_t>(longEntries_[found->second].value)
                   : std::nullopt;
    }
}
