#pragma once

#include "klass.h"

namespace splinter::engine::hotspot {
    class arrayKlassView : public klassView {
    public:
        arrayKlassView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;
    };
}