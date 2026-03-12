#include "vmConstants.h"

namespace splinter::engine::hotspot {
    void vmConstants::clear() noexcept {
        intEntries_.clear();
        longEntries_.clear();
    }

    void vmConstants::add(vmIntConstant entry) {
        intEntries_.push_back(std::move(entry));
    }

    void vmConstants::add(vmLongConstant entry) {
        longEntries_.push_back(std::move(entry));
    }

    const std::vector<vmIntConstant> &vmConstants::intEntries() const noexcept {
        return intEntries_;
    }

    const std::vector<vmLongConstant> &vmConstants::longEntries() const noexcept {
        return longEntries_;
    }

    std::optional<std::int32_t> vmConstants::findInt(std::string_view name) const noexcept {
        for (const auto &entry: intEntries_) {
            if (entry.name == name) {
                return entry.value;
            }
        }
        return std::nullopt;
    }

    std::optional<std::uint64_t> vmConstants::findLong(std::string_view name) const noexcept {
        for (const auto &entry: longEntries_) {
            if (entry.name == name) {
                return entry.value;
            }
        }
        return std::nullopt;
    }
}