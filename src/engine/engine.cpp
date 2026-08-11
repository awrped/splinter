#include "engine.h"

#include "bytecode/bytecodePrinter.h"
#include "classfile/descriptorParser.h"
#include "classfile/signatureParser.h"
#include "hotspot/classLoaderData.h"
#include "hotspot/constantPool.h"
#include "hotspot/constMethod.h"
#include "hotspot/instanceKlass.h"
#include "hotspot/klass.h"
#include "hotspot/method.h"

#include <exception>
#include <format>

namespace splinter::engine {
    bool engine::initialize(const memory::attachOptions &options) {
        lastError_.clear();

        if (!process_.attach(options)) {
            lastError_ = process_.lastError();
            return false;
        }

        if (!vm_.refresh(process_)) {
            lastError_ = vm_.lastError();
            return false;
        }

        return true;
    }

    bool engine::refreshIndexes() const {
        classIndex_.clear();
        methodIndex_.clear();
        fieldIndex_.clear();
        classNameIndex_.clear();
        methodLookupIndex_.clear();
        fieldLookupIndex_.clear();
        diagnostics_ = indexDiagnostics{};
        indexesReady_ = false;
        lastError_.clear();

        try {
            const auto processMemory = memory();
            const auto symbolReader = symbols();
            const auto klassAddresses = loadedKlassAddresses(0);

            // a member that cannot be read must not cost the whole class, so every
            // stage below fails on its own and the class is still indexed
            const auto recordSkip = [this](std::size_t &counter, const std::exception &exception) {
                ++counter;
                if (diagnostics_.firstSkipReason.empty()) {
                    diagnostics_.firstSkipReason = exception.what();
                }
            };

            diagnostics_.klassesSeen = klassAddresses.size();
            for (const auto klassAddress: klassAddresses) {
                classInfo classEntry{};
                try {
                    hotspot::klassView klass(processMemory, vm_, klassAddress);
                    classEntry.address = klass.address();
                    classEntry.name = klass.name(symbolReader);
                    classEntry.kind = klass.isInstanceKlass() ? classKind::instance : classKind::nonInstance;
                    classEntry.layoutHelper = klass.layoutHelper();
                } catch (const std::exception &exception) {
                    // without a name the entry cannot be looked up, so drop it
                    recordSkip(diagnostics_.klassesSkipped, exception);
                    continue;
                }

                if (classEntry.isInstanceKlass()) {
                    const hotspot::instanceKlassView instanceKlass(processMemory, vm_, classEntry.address);
                    std::optional<std::uint64_t> constantPoolAddress;

                    try {
                        classEntry.javaFieldCount = instanceKlass.javaFieldCount();
                        classEntry.totalFieldCount = instanceKlass.totalFieldCount();
                        constantPoolAddress = instanceKlass.constantsAddress();
                    } catch (const std::exception &exception) {
                        recordSkip(diagnostics_.membersSkipped, exception);
                    }

                    if (constantPoolAddress && *constantPoolAddress != 0) {
                        const hotspot::constantPoolView constantPool(processMemory, vm_, *constantPoolAddress);

                        std::vector<std::uint64_t> methodAddresses;
                        try {
                            methodAddresses = instanceKlass.methodAddresses();
                        } catch (const std::exception &exception) {
                            recordSkip(diagnostics_.membersSkipped, exception);
                        }
                        classEntry.methodCount = methodAddresses.size();

                        for (const auto methodAddress: methodAddresses) {
                            try {
                                hotspot::methodView method(processMemory, vm_, methodAddress);

                                methodInfo methodEntry{};
                                methodEntry.classAddress = classEntry.address;
                                methodEntry.className = classEntry.name;
                                methodEntry.address = method.address();
                                methodEntry.constMethodAddress = method.constMethodAddress().value_or(0);
                                methodEntry.methodDataAddress = method.methodDataAddress().value_or(0);
                                methodEntry.methodCountersAddress = method.methodCountersAddress().value_or(0);
                                methodEntry.name = method.name(constantPool, symbolReader);
                                methodEntry.descriptor = method.signature(constantPool, symbolReader);
                                methodEntry.displaySignature = classfile::signatureParser::parseMethod(
                                    methodEntry.descriptor);
                                methodEntry.vtableIndex = method.vtableIndex();
                                methodEntry.accessFlags = method.accessFlags();

                                methodLookupIndex_[std::format("{}#{}", methodEntry.className, methodEntry.name)]
                                        .push_back(methodIndex_.size());
                                methodIndex_.push_back(std::move(methodEntry));
                            } catch (const std::exception &exception) {
                                recordSkip(diagnostics_.membersSkipped, exception);
                            }
                        }

                        try {
                            for (const auto &decodedField: instanceKlass.fields(symbolReader, 0)) {
                                fieldInfo fieldEntry{};
                                fieldEntry.classAddress = classEntry.address;
                                fieldEntry.className = classEntry.name;
                                fieldEntry.index = decodedField.index;
                                fieldEntry.name = decodedField.name;
                                fieldEntry.descriptor = decodedField.signature;
                                fieldEntry.displayType =
                                        classfile::descriptorParser::parseField(decodedField.signature);
                                fieldEntry.genericSignature = decodedField.genericSignature;
                                fieldEntry.offset = decodedField.offset;
                                fieldEntry.accessFlags = classfile::accessFlags(decodedField.accessFlags);
                                fieldEntry.flags = decodedField.flags;
                                fieldEntry.initializerIndex = decodedField.initializerIndex;
                                fieldEntry.contentionGroup = decodedField.contentionGroup;

                                fieldLookupIndex_[std::format("{}#{}",
                                                              fieldEntry.className,
                                                              fieldEntry.name)]
                                        .push_back(fieldIndex_.size());
                                fieldIndex_.push_back(std::move(fieldEntry));
                            }
                        } catch (const std::exception &exception) {
                            recordSkip(diagnostics_.membersSkipped, exception);
                        }
                    }
                }

                classNameIndex_[classEntry.name].push_back(classIndex_.size());
                classIndex_.push_back(std::move(classEntry));
            }
        } catch (const std::exception &exception) {
            lastError_ = exception.what();
            classIndex_.clear();
            methodIndex_.clear();
            fieldIndex_.clear();
            classNameIndex_.clear();
            methodLookupIndex_.clear();
            fieldLookupIndex_.clear();
            diagnostics_ = indexDiagnostics{};
            return false;
        }

        diagnostics_.classes = classIndex_.size();
        diagnostics_.methods = methodIndex_.size();
        diagnostics_.fields = fieldIndex_.size();
        indexesReady_ = true;
        return true;
    }

    const indexDiagnostics &engine::diagnostics() const noexcept {
        return diagnostics_;
    }

    const std::string &engine::lastError() const noexcept {
        return lastError_;
    }

    const memory::remoteProcess &engine::process() const noexcept {
        return process_;
    }

    const hotspot::vmStructs &engine::vm() const noexcept {
        return vm_;
    }

    memory::processMemory engine::memory() const noexcept {
        return memory::processMemory(process_);
    }

    hotspot::symbolTable engine::symbols() const noexcept {
        return hotspot::symbolTable(memory(), vm_);
    }

    std::vector<std::uint64_t> engine::loadedKlassAddresses(std::size_t limit) const {
        const auto processMemory = memory();
        return hotspot::classLoaderDataView::enumerate(processMemory, vm_, 0, limit);
    }

    const std::vector<classInfo> &engine::classIndex() const noexcept {
        // an empty index after a failed refresh is reported through lastError
        static_cast<void>(ensureIndexes());
        return classIndex_;
    }

    const std::vector<methodInfo> &engine::methodIndex() const noexcept {
        static_cast<void>(ensureIndexes());
        return methodIndex_;
    }

    const std::vector<fieldInfo> &engine::fieldIndex() const noexcept {
        static_cast<void>(ensureIndexes());
        return fieldIndex_;
    }

    std::vector<classInfo> engine::searchClasses(std::string_view pattern, std::size_t limit) const {
        std::vector<classInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }

        for (const auto &entry: classIndex_) {
            if (pattern.empty() || entry.name.find(pattern) != std::string::npos) {
                matches.push_back(entry);
                if (limit != 0 && matches.size() >= limit) {
                    break;
                }
            }
        }
        return matches;
    }

    std::vector<classInfo> engine::findClasses(std::string_view className) const {
        std::vector<classInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }
        const auto found = classNameIndex_.find(std::string(className));
        if (found == classNameIndex_.end()) {
            return matches;
        }

        matches.reserve(found->second.size());
        for (const auto index: found->second) {
            matches.push_back(classIndex_[index]);
        }
        return matches;
    }

    std::optional<classInfo> engine::findClass(std::string_view className) const {
        if (!ensureIndexes()) {
            return std::nullopt;
        }
        const auto found = classNameIndex_.find(std::string(className));
        if (found == classNameIndex_.end() || found->second.empty()) {
            return std::nullopt;
        }
        return classIndex_[found->second.front()];
    }

    std::vector<methodInfo> engine::findMethods(std::string_view className, std::string_view methodName) const {
        std::vector<methodInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }
        const auto found = methodLookupIndex_.find(std::format("{}#{}", className, methodName));
        if (found == methodLookupIndex_.end()) {
            return matches;
        }

        matches.reserve(found->second.size());
        for (const auto index: found->second) {
            matches.push_back(methodIndex_[index]);
        }
        return matches;
    }

    std::optional<methodInfo> engine::findMethod(std::string_view className,
                                                 std::string_view methodName,
                                                 std::string_view descriptor) const {
        if (!ensureIndexes()) {
            return std::nullopt;
        }
        const auto found = methodLookupIndex_.find(std::format("{}#{}", className, methodName));
        if (found == methodLookupIndex_.end()) {
            return std::nullopt;
        }

        for (const auto index: found->second) {
            if (methodIndex_[index].descriptor == descriptor) {
                return methodIndex_[index];
            }
        }

        return std::nullopt;
    }

    std::vector<methodInfo> engine::methodsForClass(std::string_view className) const {
        std::vector<methodInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }
        for (const auto &method: methodIndex_) {
            if (method.className == className) {
                matches.push_back(method);
            }
        }
        return matches;
    }

    std::vector<fieldInfo> engine::findFields(std::string_view className, std::string_view fieldName) const {
        std::vector<fieldInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }
        const auto found = fieldLookupIndex_.find(std::format("{}#{}", className, fieldName));
        if (found == fieldLookupIndex_.end()) {
            return matches;
        }

        matches.reserve(found->second.size());
        for (const auto index: found->second) {
            matches.push_back(fieldIndex_[index]);
        }
        return matches;
    }

    std::vector<fieldInfo> engine::fieldsForClass(std::string_view className) const {
        std::vector<fieldInfo> matches;
        if (!ensureIndexes()) {
            return matches;
        }
        for (const auto &field: fieldIndex_) {
            if (field.className == className) {
                matches.push_back(field);
            }
        }
        return matches;
    }

    std::optional<fieldInfo> engine::findField(std::string_view className, std::string_view fieldName) const {
        if (!ensureIndexes()) {
            return std::nullopt;
        }
        const auto found = fieldLookupIndex_.find(std::format("{}#{}", className, fieldName));
        if (found == fieldLookupIndex_.end() || found->second.empty()) {
            return std::nullopt;
        }
        return fieldIndex_[found->second.front()];
    }

    std::optional<fieldInfo> engine::findField(std::string_view className,
                                               std::string_view fieldName,
                                               std::string_view descriptor) const {
        if (!ensureIndexes()) {
            return std::nullopt;
        }
        const auto found = fieldLookupIndex_.find(std::format("{}#{}", className, fieldName));
        if (found == fieldLookupIndex_.end()) {
            return std::nullopt;
        }

        for (const auto index: found->second) {
            if (fieldIndex_[index].descriptor == descriptor) {
                return fieldIndex_[index];
            }
        }

        return std::nullopt;
    }

    std::optional<methodDetails> engine::describeMethod(std::uint64_t methodAddress) const {
        try {
            const auto processMemory = memory();
            const auto symbolReader = symbols();
            hotspot::methodView method(processMemory, vm_, methodAddress);

            const auto constMethodAddress = method.constMethodAddress();
            if (!constMethodAddress || *constMethodAddress == 0) {
                return std::nullopt;
            }

            hotspot::constMethodView constMethod(processMemory, vm_, *constMethodAddress);
            const auto constantPoolAddress = constMethod.constantsAddress();
            if (!constantPoolAddress || *constantPoolAddress == 0) {
                return std::nullopt;
            }

            hotspot::constantPoolView constantPool(processMemory, vm_, *constantPoolAddress);

            methodDetails details{};
            details.methodAddress = method.address();
            details.constMethodAddress = *constMethodAddress;
            details.name = method.name(constantPool, symbolReader);
            details.descriptor = method.signature(constantPool, symbolReader);
            details.displaySignature = classfile::signatureParser::parseMethod(details.descriptor);
            details.codeSize = constMethod.codeSize();
            details.maxStack = constMethod.maxStack();
            details.maxLocals = constMethod.maxLocals();
            details.constMethodFlags = constMethod.flags();
            details.genericSignature = constMethod.genericSignature(constantPool, symbolReader);
            details.hasLineNumberTable = constMethod.hasLineNumberTable();
            details.hasLocalVariableTable = constMethod.hasLocalVariableTable();
            details.hasExceptionTable = constMethod.hasExceptionTable();
            details.hasCheckedExceptions = constMethod.hasCheckedExceptions();
            details.hasMethodParameters = constMethod.hasMethodParameters();
            details.hasMethodAnnotations = constMethod.hasMethodAnnotations();
            details.hasParameterAnnotations = constMethod.hasParameterAnnotations();
            details.hasTypeAnnotations = constMethod.hasTypeAnnotations();
            details.hasDefaultAnnotations = constMethod.hasDefaultAnnotations();

            if (const auto poolHolderAddress = constantPool.poolHolderAddress()) {
                hotspot::klassView holder(processMemory, vm_, *poolHolderAddress);
                details.className = holder.name(symbolReader);
            }

            for (const auto &entry: constMethod.lineNumbers()) {
                details.lineNumbers.push_back(lineNumberInfo{entry.startBci, entry.lineNumber});
            }

            for (const auto &entry: constMethod.localVariables(constantPool, symbolReader)) {
                localVariableInfo variable{};
                variable.startBci = entry.startBci;
                variable.length = entry.length;
                variable.slot = entry.slot;
                variable.name = entry.name;
                variable.descriptor = entry.descriptor;
                variable.displayType = entry.descriptor.empty()
                                           ? std::string()
                                           : classfile::descriptorParser::parseField(entry.descriptor);
                variable.genericSignature = entry.genericSignature;
                details.localVariables.push_back(std::move(variable));
            }

            for (const auto &entry: constMethod.exceptionTable(constantPool, symbolReader)) {
                details.exceptionHandlers.push_back(exceptionHandlerInfo{
                    entry.startPc,
                    entry.endPc,
                    entry.handlerPc,
                    entry.catchTypeIndex,
                    entry.catchType
                });
            }

            for (const auto &entry: constMethod.checkedExceptions(constantPool, symbolReader)) {
                details.checkedExceptions.push_back(checkedExceptionInfo{
                    entry.classIndex,
                    entry.className
                });
            }

            for (const auto &entry: constMethod.methodParameters(constantPool, symbolReader)) {
                details.parameters.push_back(methodParameterInfo{
                    entry.nameIndex,
                    entry.accessFlags,
                    entry.name
                });
            }

            const auto methodAnnotations = constMethod.methodAnnotations();
            details.methodAnnotations = annotationBlob{methodAnnotations.address, methodAnnotations.length};
            const auto parameterAnnotations = constMethod.parameterAnnotations();
            details.parameterAnnotations = annotationBlob{parameterAnnotations.address, parameterAnnotations.length};
            const auto typeAnnotations = constMethod.typeAnnotations();
            details.typeAnnotations = annotationBlob{typeAnnotations.address, typeAnnotations.length};
            const auto defaultAnnotations = constMethod.defaultAnnotations();
            details.defaultAnnotations = annotationBlob{defaultAnnotations.address, defaultAnnotations.length};

            return details;
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }

    std::optional<methodDetails> engine::describeMethod(const methodInfo &method) const {
        return describeMethod(method.address);
    }

    std::optional<disassembledMethod> engine::disassemble(std::uint64_t methodAddress) const {
        try {
            const auto processMemory = memory();
            const auto symbolReader = symbols();
            hotspot::methodView method(processMemory, vm_, methodAddress);

            const auto constMethodAddress = method.constMethodAddress();
            if (!constMethodAddress || *constMethodAddress == 0) {
                return std::nullopt;
            }

            hotspot::constMethodView constMethod(processMemory, vm_, *constMethodAddress);
            const auto constantPoolAddress = constMethod.constantsAddress();
            if (!constantPoolAddress || *constantPoolAddress == 0) {
                return std::nullopt;
            }

            hotspot::constantPoolView constantPool(processMemory, vm_, *constantPoolAddress);

            disassembledMethod result{};
            result.methodAddress = method.address();
            result.name = method.name(constantPool, symbolReader);
            result.descriptor = method.signature(constantPool, symbolReader);
            result.displaySignature = classfile::signatureParser::parseMethod(result.descriptor);

            // hotspot only rewrites a method's operands once its holder is linked
            if (const auto poolHolderAddress = constantPool.poolHolderAddress();
                poolHolderAddress && *poolHolderAddress != 0) {
                hotspot::instanceKlassView holder(processMemory, vm_, *poolHolderAddress);
                result.className = holder.name(symbolReader);
                result.rewritten = holder.isLinked().value_or(true);
            }

            result.instructions = bytecode::bytecodePrinter::decode(constMethod.bytecodes(),
                                                                    constantPool,
                                                                    symbolReader,
                                                                    result.rewritten);
            return result;
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }

    std::optional<disassembledMethod> engine::disassemble(const methodInfo &method) const {
        return disassemble(method.address);
    }

    bool engine::ensureIndexes() const {
        return indexesReady_ || refreshIndexes();
    }
}