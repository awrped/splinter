#pragma once

#include "analyzer/bytecodeAnalyzer.h"
#include "analyzer/classAnalyzer.h"
#include "analyzer/fieldAnalyzer.h"
#include "analyzer/methodAnalyzer.h"
#include "hotspot/classLoaderData.h"
#include "hotspot/constantPool.h"
#include "hotspot/instanceKlass.h"
#include "hotspot/method.h"
#include "hotspot/symbolTable.h"
#include "hotspot/vmStructs.h"
#include "memory/processMemory.h"
#include "memory/remoteProcess.h"

#include <string>
#include <vector>

namespace splinter::engine {
    class engine {
    public:
        bool initialize();

        [[nodiscard]] const std::string &lastError() const noexcept;

        [[nodiscard]] const memory::remoteProcess &process() const noexcept;

        [[nodiscard]] const hotspot::vmStructs &vm() const noexcept;

        [[nodiscard]] memory::processMemory memory() const noexcept;

        [[nodiscard]] const analyzer::classAnalyzer &classes() const noexcept;

        [[nodiscard]] const analyzer::methodAnalyzer &methods() const noexcept;

        [[nodiscard]] const analyzer::fieldAnalyzer &fields() const noexcept;

        [[nodiscard]] const analyzer::bytecodeAnalyzer &bytecode() const noexcept;

        [[nodiscard]] hotspot::symbolTable symbols() const noexcept;

        [[nodiscard]] std::vector<std::uint64_t> loadedKlassAddresses(std::size_t limit = 0) const;

    private:
        std::string lastError_;
        memory::remoteProcess process_;
        hotspot::vmStructs vm_;
        analyzer::classAnalyzer classAnalyzer_{};
        analyzer::methodAnalyzer methodAnalyzer_{};
        analyzer::fieldAnalyzer fieldAnalyzer_{};
        analyzer::bytecodeAnalyzer bytecodeAnalyzer_{};
    };
}
