#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace splinter::engine::util {
    // transparent hashing keeps string_view lookups allocation free
    struct stringHash {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view value) const noexcept {
            return std::hash<std::string_view>{}(value);
        }
    };

    template<typename value>
    using stringMap = std::unordered_map<std::string, value, stringHash, std::equal_to<> >;
}
