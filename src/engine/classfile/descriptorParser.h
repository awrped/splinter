#pragma once

#include <string>
#include <string_view>

namespace splinter::engine::classfile {
    class descriptorParser {
    public:
        [[nodiscard]] static std::string parseField(std::string_view descriptor);
    };
}