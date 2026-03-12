#pragma once

#include "../hotspot/constMethod.h"
#include "../hotspot/constantPool.h"
#include "../hotspot/method.h"
#include "../hotspot/symbolTable.h"

#include <string>

namespace splinter::engine::analyzer {
    class methodAnalyzer {
    public:
        [[nodiscard]] std::string summarize(
            const hotspot::methodView &method,
            const hotspot::constMethodView &constMethod,
            const hotspot::constantPoolView &constantPool,
            const hotspot::symbolTable &symbols) const;
    };
}