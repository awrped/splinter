#pragma once

#include "klass.h"
#include "fieldInfo.h"

#include <optional>
#include <vector>

namespace splinter::engine::hotspot {
    class symbolTable;

    class instanceKlassView : public klassView {
    public:
        instanceKlassView(const memory::processMemory &memory, const vmStructs &vm, std::uint64_t address) noexcept;

        [[nodiscard]] std::optional<std::uint64_t> constantsAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> methodsArrayAddress() const;

        [[nodiscard]] std::optional<std::uint64_t> fieldInfoStreamAddress() const;

        [[nodiscard]] std::optional<std::int32_t> javaFieldCount() const;

        [[nodiscard]] std::optional<std::int32_t> totalFieldCount() const;

        [[nodiscard]] std::vector<std::uint64_t> methodAddresses() const;

        [[nodiscard]] std::vector<decodedFieldInfo> fields(const symbolTable &symbols,
                                                           std::size_t limit = 0) const;
    };
}
