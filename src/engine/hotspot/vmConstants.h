#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace splinter::engine::hotspot {
    struct vmIntConstant {
        std::string name;
        std::int32_t value = 0;
    };

    struct vmLongConstant {
        std::string name;
        std::uint64_t value = 0;
    };

    class vmConstants {
    public:
        void clear() noexcept;

        void add(vmIntConstant entry);

        void add(vmLongConstant entry);

        [[nodiscard]] const std::vector<vmIntConstant> &intEntries() const noexcept;

        [[nodiscard]] const std::vector<vmLongConstant> &longEntries() const noexcept;

        [[nodiscard]] std::optional<std::int32_t> findInt(std::string_view name) const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> findLong(std::string_view name) const noexcept;

    private:
        std::vector<vmIntConstant> intEntries_;
        std::vector<vmLongConstant> longEntries_;
    };
}
