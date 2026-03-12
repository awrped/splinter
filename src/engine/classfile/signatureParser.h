#pragma once

#include <string>
#include <string_view>

namespace splinter::engine::classfile {
    class signatureParser {
    public:
        [[nodiscard]] static std::string parseMethod(std::string_view descriptor);
    };
}