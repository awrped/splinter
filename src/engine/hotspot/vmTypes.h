#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace splinter::engine::hotspot {
    struct vmTypeInfo {
        std::string name;
        std::string superName;
        bool isOop = false;
        bool isInteger = false;
        bool isUnsigned = false;
        std::uint64_t size = 0;
    };

    class vmTypes {
    public:
        void clear() noexcept;

        void add(vmTypeInfo entry);

        [[nodiscard]] const std::vector<vmTypeInfo> &entries() const noexcept;

        [[nodiscard]] const vmTypeInfo *find(std::string_view name) const noexcept;

        [[nodiscard]] std::optional<std::uint64_t> sizeOf(std::string_view name) const noexcept;

    private:
        std::vector<vmTypeInfo> entries_;
    };
}
