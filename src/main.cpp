#include "engine/engine.h"

#include "engine/classfile/signatureParser.h"
#include "engine/bytecode/bytecodePrinter.h"
#include "engine/hotspot/classLoaderData.h"
#include "engine/hotspot/constantPool.h"
#include "engine/hotspot/constMethod.h"
#include "engine/hotspot/instanceKlass.h"
#include "engine/hotspot/klass.h"
#include "engine/hotspot/method.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>

int main() {
    splinter::engine::engine engine;
    if (!engine.initialize()) {
        std::cerr << engine.lastError() << '\n';
        return 1;
    }

    std::cout << engine.process().describeTarget() << '\n';
    std::cout << "vmStruct fields: " << engine.vm().fields().size() << '\n';
    std::cout << "vmTypes: " << engine.vm().types().entries().size() << '\n';
    std::cout << "int constants: " << engine.vm().constants().intEntries().size() << '\n';
    std::cout << "long constants: " << engine.vm().constants().longEntries().size() << '\n';
    std::cout << "indexed classes: " << engine.classIndex().size() << '\n';
    std::cout << "indexed methods: " << engine.methodIndex().size() << '\n';
    std::cout << "indexed fields: " << engine.fieldIndex().size() << '\n';

    const auto memory = engine.memory();
    const auto symbols = engine.symbols();
    const auto klassSampleAddresses = engine.loadedKlassAddresses(8);
    const auto klassAddresses = engine.loadedKlassAddresses(4096);
    std::cout << "loaded klass sample: " << klassSampleAddresses.size() << '\n';

    std::optional<std::uint64_t> selectedInstanceKlass;
    for (std::size_t index = 0; index < klassAddresses.size(); ++index) {
        try {
            splinter::engine::hotspot::klassView klass(memory, engine.vm(), klassAddresses[index]);
            const auto layoutHelper = klass.layoutHelper();
            const bool isInstanceKlass = klass.isInstanceKlass();
            if (index < klassSampleAddresses.size()) {
                std::cout << "klass[" << index << "] 0x" << std::hex << klass.address() << std::dec;
                if (layoutHelper) {
                    std::cout << " layoutHelper=" << *layoutHelper;
                }
                std::cout << " kind=" << (isInstanceKlass ? "instance" : "non-instance")
                        << " name=" << klass.name(symbols) << '\n';
            }

            if (isInstanceKlass) {
                splinter::engine::hotspot::instanceKlassView instanceKlass(memory, engine.vm(), klass.address());
                if (instanceKlass.constantsAddress().value_or(0) != 0) {
                    const auto className = klass.name(symbols);
                    const bool isGeneratedLambdaForm = className.find("LambdaForm$") != std::string::npos;
                    if (!selectedInstanceKlass && className.find('+') == std::string::npos &&
                        !className.starts_with('[') && !isGeneratedLambdaForm) {
                        selectedInstanceKlass = klass.address();
                    }
                }
            }
        } catch (const std::exception &exception) {
            if (index < klassSampleAddresses.size()) {
                std::cout << "klass[" << index << "] 0x" << std::hex << klassAddresses[index] << std::dec
                        << " error=" << exception.what() << '\n';
            }
        }
    }

    if (!selectedInstanceKlass) {
        std::cout << "no non-generated instance klass with a constant pool was found in the current sample\n";
        return 0;
    }

    splinter::engine::hotspot::instanceKlassView klass(memory, engine.vm(), *selectedInstanceKlass);
    const auto selectedClassName = klass.name(symbols);
    std::cout << "selected instance klass: 0x" << std::hex << klass.address() << std::dec
            << " name=" << selectedClassName << '\n';
    if (const auto indexedClass = engine.findClass(selectedClassName)) {
        std::cout << "lookup class: 0x" << std::hex << indexedClass->address << std::dec
                << " methods=" << indexedClass->methodCount << '\n';
    }
    const auto constantPoolAddress = klass.constantsAddress();
    if (!constantPoolAddress) {
        std::cout << "selected klass has no constant pool\n";
        return 0;
    }

    splinter::engine::hotspot::constantPoolView constantPool(memory, engine.vm(), *constantPoolAddress);
    const auto decodedEntries = constantPool.decodeAll(symbols, 16);
    std::cout << "constant pool sample:\n";
    for (const auto &entry: decodedEntries) {
        std::cout << "  cp[" << entry.index << "] tag=" << static_cast<unsigned int>(entry.tag)
                << " " << entry.summary << '\n';
    }

    const auto methodAddresses = klass.methodAddresses();
    const auto decodedFields = klass.fields(symbols, 5);
    std::cout << "field sample: " << decodedFields.size();
    if (const auto javaFieldCount = klass.javaFieldCount()) {
        std::cout << " java=" << *javaFieldCount;
    }
    if (const auto totalFieldCount = klass.totalFieldCount()) {
        std::cout << " total=" << *totalFieldCount;
    }
    std::cout << '\n';
    for (std::size_t index = 0; index < decodedFields.size(); ++index) {
        const auto &field = decodedFields[index];
        std::cout << "  field[" << index << "] " << field.name << " " << field.signature
                << " offset=" << field.offset
                << " access=0x" << std::hex << field.accessFlags << std::dec
                << " flags=0x" << std::hex << field.flags.raw << std::dec;
        if (field.initializerIndex != 0) {
            std::cout << " init=#" << field.initializerIndex;
        }
        if (!field.genericSignature.empty()) {
            std::cout << " generic=" << field.genericSignature;
        }
        if (field.flags.isInjected()) {
            std::cout << " injected";
        }
        if (field.flags.isStable()) {
            std::cout << " stable";
        }
        if (field.flags.isContended()) {
            std::cout << " contended(" << field.contentionGroup << ")";
        }
        std::cout << '\n';
    }
    if (!decodedFields.empty()) {
        if (const auto indexedField = engine.findField(selectedClassName, decodedFields.front().name)) {
            std::cout << "lookup field: " << indexedField->name << " "
                    << indexedField->descriptor << " offset=" << indexedField->offset << '\n';
        }
    }

    std::cout << "method sample: " << std::min<std::size_t>(methodAddresses.size(), 5) << '\n';
    std::optional<std::size_t> bytecodeSampleIndex;
    for (std::size_t index = 0; index < methodAddresses.size() && index < 5; ++index) {
        splinter::engine::hotspot::methodView method(memory, engine.vm(), methodAddresses[index]);
        const auto methodName = method.name(constantPool, symbols);
        const auto signature = method.signature(constantPool, symbols);
        std::cout << "  method[" << index << "] " << methodName << " "
                << splinter::engine::classfile::signatureParser::parseMethod(signature) << '\n';
        if (index == 0) {
            if (const auto indexedMethod = engine.findMethod(selectedClassName, methodName, signature)) {
                std::cout << "  lookup method[0] 0x" << std::hex << indexedMethod->address << std::dec
                        << " " << indexedMethod->displaySignature << '\n';
            }
        }

        if (!bytecodeSampleIndex && !methodName.starts_with('<')) {
            bytecodeSampleIndex = index;
        }
    }

    if (!bytecodeSampleIndex && !methodAddresses.empty()) {
        bytecodeSampleIndex = 0;
    }

    if (bytecodeSampleIndex && *bytecodeSampleIndex < methodAddresses.size()) {
        splinter::engine::hotspot::methodView method(memory, engine.vm(), methodAddresses[*bytecodeSampleIndex]);
        const auto constMethodAddress = method.constMethodAddress();
        if (constMethodAddress) {
            splinter::engine::hotspot::constMethodView constMethod(memory, engine.vm(), *constMethodAddress);
            const auto code = constMethod.bytecodes();
            const auto instructions = splinter::engine::bytecode::bytecodePrinter::decode(code, constantPool, symbols);
            std::cout << "  decoded instructions: " << instructions.size() << '\n';
            if (!instructions.empty()) {
                std::cout << "  first instruction: " << instructions.front().mnemonic;
                if (!instructions.front().operandText.empty()) {
                    std::cout << " " << instructions.front().operandText;
                }
                std::cout << '\n';
            }
            std::cout << "  bytecode dump:\n"
                    << splinter::engine::bytecode::bytecodePrinter::print(code, constantPool, symbols);
        }
    }

    return 0;
}
