#pragma once

#include "analyzer/bytecodeAnalyzer.h"
#include "analyzer/classAnalyzer.h"
#include "analyzer/fieldAnalyzer.h"
#include "analyzer/methodAnalyzer.h"
#include "hotspot/symbolTable.h"
#include "hotspot/vmStructs.h"
#include "memory/processMemory.h"
#include "memory/remoteProcess.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace splinter::engine {
    struct classInfo {
        std::uint64_t address = 0;
        std::string name;
        bool isInstanceKlass = false;
        std::optional<std::int32_t> layoutHelper;
        std::optional<std::int32_t> javaFieldCount;
        std::optional<std::int32_t> totalFieldCount;
        std::size_t methodCount = 0;
    };

    struct methodInfo {
        std::uint64_t classAddress = 0;
        std::string className;
        std::uint64_t address = 0;
        std::string name;
        std::string signature;
        std::optional<std::uint32_t> accessFlags;
    };

    struct fieldInfo {
        std::uint64_t classAddress = 0;
        std::string className;
        hotspot::decodedFieldInfo decoded;
    };

    class engine {
    public:
        bool initialize();

        bool refreshIndexes();

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

        [[nodiscard]] const std::vector<classInfo> &classIndex() const noexcept;

        [[nodiscard]] const std::vector<methodInfo> &methodIndex() const noexcept;

        [[nodiscard]] const std::vector<fieldInfo> &fieldIndex() const noexcept;

        [[nodiscard]] std::vector<classInfo> findClasses(std::string_view className) const;

        [[nodiscard]] std::optional<classInfo> findClass(std::string_view className) const;

        [[nodiscard]] std::vector<methodInfo> findMethods(std::string_view className,
                                                          std::string_view methodName) const;

        [[nodiscard]] std::optional<methodInfo> findMethod(std::string_view className,
                                                           std::string_view methodName,
                                                           std::string_view signature) const;

        [[nodiscard]] std::vector<fieldInfo> findFields(std::string_view className,
                                                        std::string_view fieldName) const;

        [[nodiscard]] std::optional<fieldInfo> findField(std::string_view className,
                                                         std::string_view fieldName) const;

        [[nodiscard]] std::optional<fieldInfo> findField(std::string_view className,
                                                         std::string_view fieldName,
                                                         std::string_view signature) const;

    private:
        std::unordered_map<std::string, std::vector<std::size_t> > classNameIndex_;
        std::unordered_map<std::string, std::vector<std::size_t> > methodLookupIndex_;
        std::unordered_map<std::string, std::vector<std::size_t> > fieldLookupIndex_;
        std::vector<classInfo> classIndex_;
        std::vector<methodInfo> methodIndex_;
        std::vector<fieldInfo> fieldIndex_;
        std::string lastError_;
        memory::remoteProcess process_;
        hotspot::vmStructs vm_;
        analyzer::classAnalyzer classAnalyzer_{};
        analyzer::methodAnalyzer methodAnalyzer_{};
        analyzer::fieldAnalyzer fieldAnalyzer_{};
        analyzer::bytecodeAnalyzer bytecodeAnalyzer_{};
    };
}
