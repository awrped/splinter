#pragma once

#include "../hotspot/instanceKlass.h"
#include "../hotspot/symbolTable.h"

#include <string>

namespace splinter::engine::analyzer {
    class classAnalyzer {
    public:
        [[nodiscard]] std::string summarize(
            const hotspot::instanceKlassView &klass,
            const hotspot::symbolTable &symbols) const;
    };
}