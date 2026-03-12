#include "descriptorParser.h"

namespace splinter::engine::classfile {
    namespace descriptorDetail {
        std::string parseOne(std::string_view descriptor, std::size_t &index) {
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
                    const std::size_t end = descriptor.find(';', index);
                    const auto name = std::string(descriptor.substr(index, end - index));
                    index = end == std::string_view::npos ? descriptor.size() : end + 1;
                    return name;
                }
                case '[':
                    return parseOne(descriptor, index) + "[]";
                default:
                    return std::string(1, current);
            }
        }
    }

    std::string descriptorParser::parseField(std::string_view descriptor) {
        std::size_t index = 0;
        return descriptorDetail::parseOne(descriptor, index);
    }
}