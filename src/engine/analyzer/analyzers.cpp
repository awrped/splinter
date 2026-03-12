#include "bytecodeAnalyzer.h"
#include "classAnalyzer.h"
#include "fieldAnalyzer.h"
#include "methodAnalyzer.h"

#include "../bytecode/bytecodePrinter.h"
#include "../classfile/accessFlags.h"
#include "../classfile/signatureParser.h"

#include <sstream>

namespace splinter::engine::analyzer {
    std::string classAnalyzer::summarize(const hotspot::instanceKlassView &klass,
                                         const hotspot::symbolTable &symbols) const {
        std::ostringstream stream;
        stream << "klass 0x" << std::hex << klass.address() << std::dec << " name=" << klass.name(symbols);
        return stream.str();
    }

    std::string methodAnalyzer::summarize(
        const hotspot::methodView &method,
        const hotspot::constMethodView &constMethod,
        const hotspot::constantPoolView &constantPool,
        const hotspot::symbolTable &symbols) const {
        std::ostringstream stream;
        const auto nameIndex = constMethod.nameIndex();
        const auto signatureIndex = constMethod.signatureIndex();
        const auto accessBits = method.accessFlags().value_or(0);

        stream << "method 0x" << std::hex << method.address() << std::dec
                << " name=" << (nameIndex ? constantPool.utf8At(*nameIndex, symbols) : "")
                << " signature=" << (signatureIndex
                                         ? classfile::signatureParser::parseMethod(
                                             constantPool.utf8At(*signatureIndex, symbols))
                                         : "")
                << " access=0x" << std::hex << accessBits << std::dec;
        return stream.str();
    }

    std::string fieldAnalyzer::summarize(const hotspot::fieldInfoView &field) const {
        std::ostringstream stream;
        stream << "fieldInfo 0x" << std::hex << field.address() << std::dec
                << " javaFields=" << field.javaFieldCount().value_or(0)
                << " totalFields=" << field.totalFieldCount().value_or(0);
        return stream.str();
    }

    std::string bytecodeAnalyzer::summarize(const std::vector<std::uint8_t> &code) const {
        return bytecode::bytecodePrinter::print(code);
    }
}