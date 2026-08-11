#include "descriptorParser.h"

#include <algorithm>

namespace splinter::engine::classfile {
    std::string descriptorParser::parseField(std::string_view descriptor) {
        std::size_t index = 0;
        return parseNext(descriptor, index);
    }

    std::string descriptorParser::parseNext(std::string_view descriptor, std::size_t &index) {
        if (index >= descriptor.size()) {
            return {};
        }

        const char current = descriptor[index++];
        switch (current) {
            case 'B': return "byte";
            case 'C': return "char";
            case 'D': return "double";
            case 'F': return "float";
            case 'I': return "int";
            case 'J': return "long";
            case 'S': return "short";
            case 'Z': return "boolean";
            case 'V': return "void";
            case 'L': {
                const std::size_t terminator = descriptor.find(';', index);
                const std::size_t end = terminator == std::string_view::npos ? descriptor.size() : terminator;
                std::string name(descriptor.substr(index, end - index));
                std::replace(name.begin(), name.end(), '/', '.');
                index = terminator == std::string_view::npos ? descriptor.size() : terminator + 1;
                return name;
            }
            case '[':
                return parseNext(descriptor, index) + "[]";
            default:
                return std::string(1, current);
        }
    }
}
