#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace splinter::engine::classfile {
    class descriptorParser {
    public:
        [[nodiscard]] static std::string parseField(std::string_view descriptor);

        // parses one field descriptor and advances index past it, index is left
        // untouched when the descriptor is exhausted
        [[nodiscard]] static std::string parseNext(std::string_view descriptor, std::size_t &index);
    };
}
