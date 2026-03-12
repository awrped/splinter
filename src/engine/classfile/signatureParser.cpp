#include "signatureParser.h"

#include "descriptorParser.h"

#include <sstream>

namespace splinter::engine::classfile {
    std::string signatureParser::parseMethod(std::string_view descriptor) {
        if (descriptor.empty() || descriptor.front() != '(') {
            return std::string(descriptor);
        }

        std::ostringstream stream;
        std::size_t index = 1;
        stream << '(';
        bool first = true;
        while (index < descriptor.size() && descriptor[index] != ')') {
            if (!first) {
                stream << ", ";
            }
            const std::size_t start = index;
            stream << descriptorParser::parseField(descriptor.substr(start));
            if (descriptor[start] == 'L') {
                index = descriptor.find(';', start) + 1;
            } else if (descriptor[start] == '[') {
                while (descriptor[index] == '[') {
                    ++index;
                }
                if (descriptor[index] == 'L') {
                    index = descriptor.find(';', index) + 1;
                } else {
                    ++index;
                }
            } else {
                ++index;
            }
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