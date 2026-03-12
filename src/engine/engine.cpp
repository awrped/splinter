#include "engine.h"

#include "hotspot/classLoaderData.h"
#include "hotspot/constantPool.h"
#include "hotspot/instanceKlass.h"
#include "hotspot/klass.h"
#include "hotspot/method.h"

#include <exception>
#include <format>

namespace splinter::engine {
    bool engine::initialize() {
        lastError_.clear();

        if (!process_.attachToJavaw()) {
            lastError_ = process_.lastError();
            return false;
        }

        if (!vm_.refresh(process_)) {
            lastError_ = vm_.lastError();
            return false;
        }

        return refreshIndexes();
    }

    bool engine::refreshIndexes() {
        classIndex_.clear();
        methodIndex_.clear();
        fieldIndex_.clear();
        classNameIndex_.clear();
        methodLookupIndex_.clear();
        fieldLookupIndex_.clear();
        lastError_.clear();

        try {
            const auto processMemory = memory();
            const auto symbolReader = symbols();
            const auto klassAddresses = loadedKlassAddresses(0);

            for (const auto klassAddress: klassAddresses) {
                try {
                    hotspot::klassView klass(processMemory, vm_, klassAddress);

                    classInfo classEntry{};
                    classEntry.address = klass.address();
                    classEntry.name = klass.name(symbolReader);
                    classEntry.isInstanceKlass = klass.isInstanceKlass();
                    classEntry.layoutHelper = klass.layoutHelper();

                    if (classEntry.isInstanceKlass) {
                        hotspot::instanceKlassView instanceKlass(processMemory, vm_, klass.address());
                        classEntry.javaFieldCount = instanceKlass.javaFieldCount();
                        classEntry.totalFieldCount = instanceKlass.totalFieldCount();

                        const auto constantPoolAddress = instanceKlass.constantsAddress();
                        if (constantPoolAddress && *constantPoolAddress != 0) {
                            const hotspot::constantPoolView constantPool(processMemory, vm_, *constantPoolAddress);

                            const auto methodAddresses = instanceKlass.methodAddresses();
                            classEntry.methodCount = methodAddresses.size();

                            for (const auto methodAddress: methodAddresses) {
                                hotspot::methodView method(processMemory, vm_, methodAddress);

                                methodInfo methodEntry{};
                                methodEntry.classAddress = classEntry.address;
                                methodEntry.className = classEntry.name;
                                methodEntry.address = method.address();
                                methodEntry.name = method.name(constantPool, symbolReader);
                                methodEntry.signature = method.signature(constantPool, symbolReader);
                                methodEntry.accessFlags = method.accessFlags();

                                methodLookupIndex_[std::format("{}#{}", methodEntry.className, methodEntry.name)]
                                        .push_back(methodIndex_.size());
                                methodIndex_.push_back(std::move(methodEntry));
                            }

                            for (auto decodedField: instanceKlass.fields(symbolReader, 0)) {
                                fieldInfo fieldEntry{};
                                fieldEntry.classAddress = classEntry.address;
                                fieldEntry.className = classEntry.name;
                                fieldEntry.decoded = std::move(decodedField);

                                fieldLookupIndex_[std::format("{}#{}",
                                                              fieldEntry.className,
                                                              fieldEntry.decoded.name)]
                                        .push_back(fieldIndex_.size());
                                fieldIndex_.push_back(std::move(fieldEntry));
                            }
                        }
                    }

                    classNameIndex_[classEntry.name].push_back(classIndex_.size());
                    classIndex_.push_back(std::move(classEntry));
                } catch (const std::exception &) {
                }
            }
        } catch (const std::exception &exception) {
            lastError_ = exception.what();
            classIndex_.clear();
            methodIndex_.clear();
            fieldIndex_.clear();
            classNameIndex_.clear();
            methodLookupIndex_.clear();
            fieldLookupIndex_.clear();
            return false;
        }

        return true;
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

    const analyzer::classAnalyzer &engine::classes() const noexcept {
        return classAnalyzer_;
    }

    const analyzer::methodAnalyzer &engine::methods() const noexcept {
        return methodAnalyzer_;
    }

    const analyzer::fieldAnalyzer &engine::fields() const noexcept {
        return fieldAnalyzer_;
    }

    const analyzer::bytecodeAnalyzer &engine::bytecode() const noexcept {
        return bytecodeAnalyzer_;
    }

    hotspot::symbolTable engine::symbols() const noexcept {
        return hotspot::symbolTable(memory(), vm_);
    }

    std::vector<std::uint64_t> engine::loadedKlassAddresses(std::size_t limit) const {
        const auto processMemory = memory();
        return hotspot::classLoaderDataView::enumerate(processMemory, vm_, 0, limit);
    }

    const std::vector<classInfo> &engine::classIndex() const noexcept {
        return classIndex_;
    }

    const std::vector<methodInfo> &engine::methodIndex() const noexcept {
        return methodIndex_;
    }

    const std::vector<fieldInfo> &engine::fieldIndex() const noexcept {
        return fieldIndex_;
    }

    std::vector<classInfo> engine::findClasses(std::string_view className) const {
        std::vector<classInfo> matches;
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
        const auto found = classNameIndex_.find(std::string(className));
        if (found == classNameIndex_.end() || found->second.empty()) {
            return std::nullopt;
        }
        return classIndex_[found->second.front()];
    }

    std::vector<methodInfo> engine::findMethods(std::string_view className, std::string_view methodName) const {
        std::vector<methodInfo> matches;
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
                                                 std::string_view signature) const {
        const auto found = methodLookupIndex_.find(std::format("{}#{}", className, methodName));
        if (found == methodLookupIndex_.end()) {
            return std::nullopt;
        }

        for (const auto index: found->second) {
            if (methodIndex_[index].signature == signature) {
                return methodIndex_[index];
            }
        }

        return std::nullopt;
    }

    std::vector<fieldInfo> engine::findFields(std::string_view className, std::string_view fieldName) const {
        std::vector<fieldInfo> matches;
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

    std::optional<fieldInfo> engine::findField(std::string_view className, std::string_view fieldName) const {
        const auto found = fieldLookupIndex_.find(std::format("{}#{}", className, fieldName));
        if (found == fieldLookupIndex_.end() || found->second.empty()) {
            return std::nullopt;
        }
        return fieldIndex_[found->second.front()];
    }

    std::optional<fieldInfo> engine::findField(std::string_view className,
                                               std::string_view fieldName,
                                               std::string_view signature) const {
        const auto found = fieldLookupIndex_.find(std::format("{}#{}", className, fieldName));
        if (found == fieldLookupIndex_.end()) {
            return std::nullopt;
        }

        for (const auto index: found->second) {
            if (fieldIndex_[index].decoded.signature == signature) {
                return fieldIndex_[index];
            }
        }

        return std::nullopt;
    }
}
