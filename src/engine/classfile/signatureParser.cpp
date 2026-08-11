#include "signatureParser.h"

#include "descriptorParser.h"

#include <sstream>

namespace splinter::engine::classfile {
    std::string signatureParser::parseMethod(std::string_view descriptor) {
        if (descriptor.empty() || descriptor.front() != '(') {
            return std::string(descriptor);
        }

        std::ostringstream stream;
        stream << '(';

        std::size_t index = 1;
        bool first = true;
        while (index < descriptor.size() && descriptor[index] != ')') {
            const std::size_t start = index;
            const auto parameter = descriptorParser::parseNext(descriptor, index);
            if (index == start) {
                // nothing was consumed, the descriptor is malformed
                break;
            }

            if (!first) {
                stream << ", ";
            }
            stream << parameter;
            first = false;
        }

        stream << ')';

        if (index < descriptor.size() && descriptor[index] == ')') {
            ++index;
        }

        stream << " -> " << descriptorParser::parseField(descriptor.substr(index));
        return stream.str();
    }
}
