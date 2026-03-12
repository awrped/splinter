#pragma once

#include "analyzer/bytecodeAnalyzer.h"
#include "analyzer/classAnalyzer.h"
#include "analyzer/fieldAnalyzer.h"
#include "analyzer/methodAnalyzer.h"
#include "classfile/accessFlags.h"
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
    enum class classKind {
        instance,
        nonInstance
    };

    struct classInfo {
        std::uint64_t address = 0;
        std::string name;
        classKind kind = classKind::nonInstance;
        std::optional<std::int32_t> layoutHelper;
        std::optional<std::int32_t> javaFieldCount;
        std::optional<std::int32_t> totalFieldCount;
        std::size_t methodCount = 0;

        [[nodiscard]] bool isInstanceKlass() const noexcept {
            return kind == classKind::instance;
        }
    };

    struct methodInfo {
        std::uint64_t classAddress = 0;
        std::string className;
        std::uint64_t address = 0;
        std::uint64_t constMethodAddress = 0;
        std::uint64_t methodDataAddress = 0;
        std::uint64_t methodCountersAddress = 0;
        std::string name;
        std::string descriptor;
        std::string displaySignature;
        std::optional<std::int32_t> vtableIndex;
        std::optional<std::uint32_t> accessFlags;
    };

    struct lineNumberInfo {
        std::uint16_t startBci = 0;
        std::uint16_t lineNumber = 0;
    };

    struct localVariableInfo {
        std::uint16_t startBci = 0;
        std::uint16_t length = 0;
        std::uint16_t slot = 0;
        std::string name;
        std::string descriptor;
        std::string displayType;
        std::string genericSignature;
    };

    struct exceptionHandlerInfo {
        std::uint16_t startPc = 0;
        std::uint16_t endPc = 0;
        std::uint16_t handlerPc = 0;
        std::uint16_t catchTypeIndex = 0;
        std::string catchType;
    };

    struct checkedExceptionInfo {
        std::uint16_t classIndex = 0;
        std::string className;
    };

    struct methodParameterInfo {
        std::uint16_t nameIndex = 0;
        std::uint16_t accessFlags = 0;
        std::string name;
    };

    struct annotationBlob {
        std::uint64_t address = 0;
        std::int32_t length = 0;
    };

    struct methodDetails {
        std::uint64_t methodAddress = 0;
        std::uint64_t constMethodAddress = 0;
        std::string className;
        std::string name;
        std::string descriptor;
        std::string displaySignature;
        std::optional<std::uint16_t> codeSize;
        std::optional<std::uint16_t> maxStack;
        std::optional<std::uint16_t> maxLocals;
        std::optional<std::uint32_t> constMethodFlags;
        std::string genericSignature;
        bool hasLineNumberTable = false;
        bool hasLocalVariableTable = false;
        bool hasExceptionTable = false;
        bool hasCheckedExceptions = false;
        bool hasMethodParameters = false;
        bool hasMethodAnnotations = false;
        bool hasParameterAnnotations = false;
        bool hasTypeAnnotations = false;
        bool hasDefaultAnnotations = false;
        annotationBlob methodAnnotations{};
        annotationBlob parameterAnnotations{};
        annotationBlob typeAnnotations{};
        annotationBlob defaultAnnotations{};
        std::vector<lineNumberInfo> lineNumbers;
        std::vector<localVariableInfo> localVariables;
        std::vector<exceptionHandlerInfo> exceptionHandlers;
        std::vector<checkedExceptionInfo> checkedExceptions;
        std::vector<methodParameterInfo> parameters;
    };

    struct fieldInfo {
        std::uint64_t classAddress = 0;
        std::string className;
        std::uint32_t index = 0;
        std::string name;
        std::string descriptor;
        std::string displayType;
        std::string genericSignature;
        std::uint32_t offset = 0;
        classfile::accessFlags accessFlags{};
        hotspot::fieldFlags flags{};
        std::uint16_t initializerIndex = 0;
        std::uint16_t contentionGroup = 0;
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

        [[nodiscard]] std::vector<methodInfo> methodsForClass(std::string_view className) const;

        [[nodiscard]] std::optional<methodInfo> findMethod(std::string_view className,
                                                           std::string_view methodName,
                                                           std::string_view descriptor) const;

        [[nodiscard]] std::vector<fieldInfo> findFields(std::string_view className,
                                                        std::string_view fieldName) const;

        [[nodiscard]] std::vector<fieldInfo> fieldsForClass(std::string_view className) const;

        [[nodiscard]] std::optional<fieldInfo> findField(std::string_view className,
                                                         std::string_view fieldName) const;

        [[nodiscard]] std::optional<fieldInfo> findField(std::string_view className,
                                                         std::string_view fieldName,
                                                         std::string_view descriptor) const;

        [[nodiscard]] std::optional<methodDetails> describeMethod(std::uint64_t methodAddress) const;

        [[nodiscard]] std::optional<methodDetails> describeMethod(const methodInfo &method) const;

    private:
        [[nodiscard]] bool ensureIndexes() const;

        std::unordered_map<std::string, std::vector<std::size_t> > classNameIndex_;
        std::unordered_map<std::string, std::vector<std::size_t> > methodLookupIndex_;
        std::unordered_map<std::string, std::vector<std::size_t> > fieldLookupIndex_;
        std::vector<classInfo> classIndex_;
        std::vector<methodInfo> methodIndex_;
        std::vector<fieldInfo> fieldIndex_;
        bool indexesReady_ = false;
        std::string lastError_;
        memory::remoteProcess process_;
        hotspot::vmStructs vm_;
        analyzer::classAnalyzer classAnalyzer_{};
        analyzer::methodAnalyzer methodAnalyzer_{};
        analyzer::fieldAnalyzer fieldAnalyzer_{};
        analyzer::bytecodeAnalyzer bytecodeAnalyzer_{};
    };
}
