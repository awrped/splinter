#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace splinter::engine::memory {
    struct moduleInfo {
        std::wstring name;
        std::wstring path;
        std::uint64_t base = 0;
        std::uint64_t size = 0;
    };

    class moduleMap {
    public:
        void clear() noexcept;

        void add(moduleInfo module);

        [[nodiscard]] const std::vector<moduleInfo> &modules() const noexcept;

        [[nodiscard]] const moduleInfo *findByName(std::wstring_view moduleName) const noexcept;

    private:
        std::vector<moduleInfo> modules_;
    };
}
