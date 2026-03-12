#pragma once

#include "../hotspot/fieldInfo.h"

#include <string>

namespace splinter::engine::analyzer {
    class fieldAnalyzer {
    public:
        [[nodiscard]] std::string summarize(const hotspot::fieldInfoView &field) const;
    };
}