#include "bytecodePrinter.h"

#include "bytecodeTable.h"
#include "../hotspot/constantPoolCache.h"

#include <bit>
#include <iomanip>
#include <sstream>

namespace splinter::engine::bytecode {
    namespace bytecodePrinterDetail {
        template<typename... Args>
        [[nodiscard]] std::string joinWithSpaces(Args &&... args) {
            std::ostringstream stream;
            bool first = true;
            ((stream << (first ? "" : " ") << std::forward<Args>(args), first = false), ...);
            return stream.str();
        }

        [[nodiscard]] std::uint16_t readU16(const std::vector<std::uint8_t> &code, std::size_t offset) {
            if (offset + 1 >= code.size()) {
                return 0;
            }
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(code[offset]) << 8U) |
                                              static_cast<std::uint16_t>(code[offset + 1]));
        }

        [[nodiscard]] std::int16_t readS16(const std::vector<std::uint8_t> &code, std::size_t offset) {
            return static_cast<std::int16_t>(readU16(code, offset));
        }

        [[nodiscard]] std::uint32_t readU32(const std::vector<std::uint8_t> &code, std::size_t offset) {
            if (offset + 3 >= code.size()) {
                return 0;
            }
            return (static_cast<std::uint32_t>(code[offset]) << 24U) |
                   (static_cast<std::uint32_t>(code[offset + 1]) << 16U) |
                   (static_cast<std::uint32_t>(code[offset + 2]) << 8U) |
                   static_cast<std::uint32_t>(code[offset + 3]);
        }

        [[nodiscard]] std::int32_t readS32(const std::vector<std::uint8_t> &code, std::size_t offset) {
            return static_cast<std::int32_t>(readU32(code, offset));
        }

        [[nodiscard]] std::uint16_t readNativeU16(const std::vector<std::uint8_t> &code, std::size_t offset) {
            if constexpr (std::endian::native == std::endian::little) {
                if (offset + 1 >= code.size()) {
                    return 0;
                }
                return static_cast<std::uint16_t>(static_cast<std::uint16_t>(code[offset]) |
                                                  (static_cast<std::uint16_t>(code[offset + 1]) << 8U));
            } else {
                return readU16(code, offset);
            }
        }

        [[nodiscard]] std::uint32_t readNativeU32(const std::vector<std::uint8_t> &code, std::size_t offset) {
            if constexpr (std::endian::native == std::endian::little) {
                if (offset + 3 >= code.size()) {
                    return 0;
                }
                return static_cast<std::uint32_t>(code[offset]) |
                       (static_cast<std::uint32_t>(code[offset + 1]) << 8U) |
                       (static_cast<std::uint32_t>(code[offset + 2]) << 16U) |
                       (static_cast<std::uint32_t>(code[offset + 3]) << 24U);
            } else {
                return readU32(code, offset);
            }
        }

        [[nodiscard]] std::size_t tableSwitchLength(const std::vector<std::uint8_t> &code, std::size_t index) {
            const std::size_t aligned = (index + 4U) & ~std::size_t(3);
            if (aligned + 11 >= code.size()) {
                return code.size() - index;
            }

            const auto low = readS32(code, aligned + 4);
            const auto high = readS32(code, aligned + 8);
            if (high < low) {
                return aligned + 12U - index;
            }

            return (aligned - index) + 12U + static_cast<std::size_t>(high - low + 1) * 4U;
        }

        [[nodiscard]] std::size_t lookupSwitchLength(const std::vector<std::uint8_t> &code, std::size_t index) {
            const std::size_t aligned = (index + 4U) & ~std::size_t(3);
            if (aligned + 7 >= code.size()) {
                return code.size() - index;
            }

            const auto pairs = readS32(code, aligned + 4);
            if (pairs < 0) {
                return aligned + 8U - index;
            }

            return (aligned - index) + 8U + static_cast<std::size_t>(pairs) * 8U;
        }

        [[nodiscard]] std::size_t instructionLength(const std::vector<std::uint8_t> &code, std::size_t index) {
            if (index >= code.size()) {
                return 0;
            }

            switch (code[index]) {
                case 0x10:
                case 0x12:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3a:
                case 0xa9:
                case 0xbc:
                case 0xe0:
                case 0xe2:
                case 0xe6:
                    return 2;
                case 0x11:
                case 0x13:
                case 0x14:
                case 0x99:
                case 0x9a:
                case 0x9b:
                case 0x9c:
                case 0x9d:
                case 0x9e:
                case 0x9f:
                case 0xa0:
                case 0xa1:
                case 0xa2:
                case 0xa3:
                case 0xa4:
                case 0xa5:
                case 0xa6:
                case 0xa7:
                case 0xa8:
                case 0xb2:
                case 0xb3:
                case 0xb4:
                case 0xb5:
                case 0xb6:
                case 0xb7:
                case 0xb8:
                case 0xbb:
                case 0xbd:
                case 0xc0:
                case 0xc1:
                case 0xc5:
                case 0xc6:
                case 0xc7:
                case 0xcb:
                case 0xcc:
                case 0xcd:
                case 0xce:
                case 0xcf:
                case 0xd0:
                case 0xd1:
                case 0xd2:
                case 0xd3:
                case 0xd4:
                case 0xd5:
                case 0xd6:
                case 0xd7:
                case 0xd8:
                case 0xd9:
                case 0xda:
                case 0xdb:
                case 0xe3:
                case 0xe7:
                case 0xe9:
                    return 3;
                case 0x84:
                    return 3;
                case 0xb9:
                case 0xba:
                case 0xc8:
                case 0xc9:
                    return 5;
                case 0xdd:
                case 0xde:
                case 0xdf:
                case 0xe1:
                    return 4;
                case 0xaa:
                case 0xe4:
                    return tableSwitchLength(code, index);
                case 0xab:
                case 0xe5:
                    return lookupSwitchLength(code, index);
                case 0xc4:
                    if (index + 1 >= code.size()) {
                        return 1;
                    }
                    return code[index + 1] == 0x84 ? 6 : 4;
                default:
                    return 1;
            }
        }

        [[nodiscard]] std::string decodeConstantPoolOperand(const hotspot::constantPoolView &constantPool,
                                                            const hotspot::symbolTable &symbols,
                                                            std::uint32_t constantPoolIndex) {
            if (constantPoolIndex == 0) {
                return "#0";
            }

            const auto entry = constantPool.decodeAt(static_cast<std::int32_t>(constantPoolIndex), symbols);
            if (entry.summary.empty()) {
                return "#" + std::to_string(constantPoolIndex);
            }
            return joinWithSpaces("#" + std::to_string(constantPoolIndex), entry.summary);
        }

        [[nodiscard]] std::string decodeResolvedReferenceOperand(const hotspot::constantPoolView &constantPool,
                                                                 const hotspot::symbolTable &symbols,
                                                                 std::uint16_t referenceIndex) {
            const auto cacheAddress = constantPool.cacheAddress();
            if (!cacheAddress || *cacheAddress == 0) {
                return "refIndex=" + std::to_string(referenceIndex);
            }

            const hotspot::constantPoolCacheView cacheView(constantPool.memory(), constantPool.vm(), *cacheAddress);
            const auto cpIndex = cacheView.objectConstantPoolIndexAt(referenceIndex);
            if (!cpIndex) {
                return "refIndex=" + std::to_string(referenceIndex);
            }

            return decodeConstantPoolOperand(constantPool, symbols, *cpIndex);
        }

        [[nodiscard]] std::string decodeInstruction(const std::vector<std::uint8_t> &code,
                                                    std::size_t index,
                                                    const hotspot::constantPoolView *constantPool,
                                                    const hotspot::symbolTable *symbols) {
            const auto opcode = code[index];
            switch (opcode) {
                case 0x10:
                    return "value=" + std::to_string(
                               static_cast<std::int32_t>(static_cast<std::int8_t>(code[index + 1])));
                case 0x11:
                    return "value=" + std::to_string(static_cast<std::int32_t>(readS16(code, index + 1)));
                case 0x12:
                    if (constantPool != nullptr && symbols != nullptr) {
                        return decodeConstantPoolOperand(*constantPool, *symbols, code[index + 1]);
                    }
                    return "cp=#" + std::to_string(code[index + 1]);
                case 0x13:
                case 0x14:
                case 0xbb:
                case 0xbd:
                case 0xc0:
                case 0xc1:
                    if (constantPool != nullptr && symbols != nullptr) {
                        return decodeConstantPoolOperand(*constantPool, *symbols, readU16(code, index + 1));
                    }
                    return "cp=#" + std::to_string(readU16(code, index + 1));
                case 0xb2:
                case 0xb3:
                case 0xb4:
                case 0xb5:
                case 0xcb:
                case 0xcc:
                case 0xcd:
                case 0xce:
                case 0xcf:
                case 0xd0:
                case 0xd1:
                case 0xd2:
                case 0xd3:
                case 0xd4:
                case 0xd5:
                case 0xd6:
                case 0xd7:
                case 0xd8:
                case 0xd9:
                case 0xda:
                case 0xdb: {
                    const auto fieldIndex = readNativeU16(code, index + 1);
                    if (constantPool != nullptr && symbols != nullptr) {
                        const auto cacheAddress = constantPool->cacheAddress();
                        if (cacheAddress && *cacheAddress != 0) {
                            const hotspot::constantPoolCacheView cacheView(constantPool->memory(),
                                                                           constantPool->vm(),
                                                                           *cacheAddress);
                            const auto cpIndex = cacheView.fieldConstantPoolIndexAt(fieldIndex);
                            if (cpIndex) {
                                return decodeConstantPoolOperand(*constantPool, *symbols, *cpIndex);
                            }
                        }
                    }
                    return "fieldIndex=" + std::to_string(fieldIndex);
                }
                case 0xdd:
                case 0xde:
                case 0xdf: {
                    const auto fieldIndex = readNativeU16(code, index + 2);
                    if (constantPool != nullptr && symbols != nullptr) {
                        const auto cacheAddress = constantPool->cacheAddress();
                        if (cacheAddress && *cacheAddress != 0) {
                            const hotspot::constantPoolCacheView cacheView(constantPool->memory(),
                                                                           constantPool->vm(),
                                                                           *cacheAddress);
                            const auto cpIndex = cacheView.fieldConstantPoolIndexAt(fieldIndex);
                            if (cpIndex) {
                                return decodeConstantPoolOperand(*constantPool, *symbols, *cpIndex);
                            }
                        }
                    }
                    return "fieldIndex=" + std::to_string(fieldIndex);
                }
                case 0xb6:
                case 0xb7:
                case 0xb8:
                case 0xe3:
                case 0xe9: {
                    const auto methodIndex = readNativeU16(code, index + 1);
                    if (constantPool != nullptr && symbols != nullptr) {
                        const auto cacheAddress = constantPool->cacheAddress();
                        if (cacheAddress && *cacheAddress != 0) {
                            const hotspot::constantPoolCacheView cacheView(constantPool->memory(),
                                                                           constantPool->vm(),
                                                                           *cacheAddress);
                            const auto cpIndex = cacheView.methodConstantPoolIndexAt(methodIndex);
                            if (cpIndex) {
                                return decodeConstantPoolOperand(*constantPool, *symbols, *cpIndex);
                            }
                        }
                    }
                    return "methodIndex=" + std::to_string(methodIndex);
                }
                case 0xb9: {
                    const auto methodIndex = readNativeU16(code, index + 1);
                    const auto argumentCount = code[index + 3];
                    if (constantPool != nullptr && symbols != nullptr) {
                        const auto cacheAddress = constantPool->cacheAddress();
                        if (cacheAddress && *cacheAddress != 0) {
                            const hotspot::constantPoolCacheView cacheView(constantPool->memory(),
                                                                           constantPool->vm(),
                                                                           *cacheAddress);
                            const auto cpIndex = cacheView.methodConstantPoolIndexAt(methodIndex);
                            if (cpIndex) {
                                return joinWithSpaces(decodeConstantPoolOperand(*constantPool, *symbols, *cpIndex),
                                                      "count=" + std::to_string(argumentCount));
                            }
                        }
                    }
                    return joinWithSpaces("methodIndex=" + std::to_string(methodIndex),
                                          "count=" + std::to_string(argumentCount));
                }
                case 0xba: {
                    const auto encodedIndex = readNativeU32(code, index + 1);
                    if (constantPool != nullptr && symbols != nullptr) {
                        const auto cacheAddress = constantPool->cacheAddress();
                        if (cacheAddress && *cacheAddress != 0) {
                            const hotspot::constantPoolCacheView cacheView(constantPool->memory(),
                                                                           constantPool->vm(),
                                                                           *cacheAddress);
                            const auto cpIndex =
                                    cacheView.indyConstantPoolIndexAt(
                                        hotspot::constantPoolCacheView::decodeInvokedynamicIndex(encodedIndex));
                            if (cpIndex) {
                                return decodeConstantPoolOperand(*constantPool, *symbols, *cpIndex);
                            }
                        }
                    }
                    return "indyIndex=" +
                           std::to_string(hotspot::constantPoolCacheView::decodeInvokedynamicIndex(encodedIndex));
                }
                case 0xe6:
                    if (constantPool != nullptr && symbols != nullptr) {
                        return decodeResolvedReferenceOperand(*constantPool, *symbols, code[index + 1]);
                    }
                    return "refIndex=" + std::to_string(code[index + 1]);
                case 0xe7:
                    if (constantPool != nullptr && symbols != nullptr) {
                        return decodeResolvedReferenceOperand(*constantPool, *symbols, readNativeU16(code, index + 1));
                    }
                    return "refIndex=" + std::to_string(readNativeU16(code, index + 1));
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3a:
                case 0xa9:
                case 0xe0:
                case 0xe2:
                    return "local=" + std::to_string(code[index + 1]);
                case 0xe1:
                    return joinWithSpaces("local0=" + std::to_string(code[index + 1]),
                                          "local1=" + std::to_string(code[index + 3]));
                case 0x84:
                    return joinWithSpaces("local=" + std::to_string(code[index + 1]),
                                          "delta=" + std::to_string(static_cast<std::int32_t>(
                                              static_cast<std::int8_t>(code[index + 2]))));
                case 0x99:
                case 0x9a:
                case 0x9b:
                case 0x9c:
                case 0x9d:
                case 0x9e:
                case 0x9f:
                case 0xa0:
                case 0xa1:
                case 0xa2:
                case 0xa3:
                case 0xa4:
                case 0xa5:
                case 0xa6:
                case 0xa7:
                case 0xa8:
                case 0xc6:
                case 0xc7:
                    return "target=" + std::to_string(static_cast<std::int64_t>(index) + readS16(code, index + 1));
                case 0xc8:
                case 0xc9:
                    return "target=" + std::to_string(static_cast<std::int64_t>(index) + readS32(code, index + 1));
                case 0xbc:
                    switch (code[index + 1]) {
                        case 4: return "atype=boolean";
                        case 5: return "atype=char";
                        case 6: return "atype=float";
                        case 7: return "atype=double";
                        case 8: return "atype=byte";
                        case 9: return "atype=short";
                        case 10: return "atype=int";
                        case 11: return "atype=long";
                        default: return "atype=" + std::to_string(code[index + 1]);
                    }
                case 0xc4:
                    if (index + 1 >= code.size()) {
                        return {};
                    }
                    if (code[index + 1] == 0x84) {
                        return joinWithSpaces("wide", "iinc",
                                              "local=" + std::to_string(readU16(code, index + 2)),
                                              "delta=" + std::to_string(readS16(code, index + 4)));
                    }
                    return joinWithSpaces("wide",
                                          std::string(bytecodeTable::name(code[index + 1])),
                                          "local=" + std::to_string(readU16(code, index + 2)));
                case 0xc5:
                    if (constantPool != nullptr && symbols != nullptr) {
                        return joinWithSpaces(
                            decodeConstantPoolOperand(*constantPool, *symbols, readU16(code, index + 1)),
                            "dimensions=" + std::to_string(code[index + 3]));
                    }
                    return joinWithSpaces("cp=#" + std::to_string(readU16(code, index + 1)),
                                          "dimensions=" + std::to_string(code[index + 3]));
                case 0xaa:
                case 0xe4: {
                    const std::size_t aligned = (index + 4U) & ~std::size_t(3);
                    const auto defaultTarget = static_cast<std::int64_t>(index) + readS32(code, aligned);
                    const auto low = readS32(code, aligned + 4);
                    const auto high = readS32(code, aligned + 8);
                    return joinWithSpaces("default=" + std::to_string(defaultTarget),
                                          "low=" + std::to_string(low),
                                          "high=" + std::to_string(high));
                }
                case 0xab:
                case 0xe5: {
                    const std::size_t aligned = (index + 4U) & ~std::size_t(3);
                    const auto defaultTarget = static_cast<std::int64_t>(index) + readS32(code, aligned);
                    const auto pairs = readS32(code, aligned + 4);
                    return joinWithSpaces("default=" + std::to_string(defaultTarget),
                                          "pairs=" + std::to_string(pairs));
                }
                default:
                    return {};
            }
        }

        [[nodiscard]] std::vector<instructionInfo> decodeImpl(const std::vector<std::uint8_t> &code,
                                                              const hotspot::constantPoolView *constantPool,
                                                              const hotspot::symbolTable *symbols) {
            std::vector<instructionInfo> instructions;
            std::size_t index = 0;
            while (index < code.size()) {
                const auto opcode = code[index];
                const auto length = instructionLength(code, index);

                instructionInfo instruction{};
                instruction.offset = index;
                instruction.opcode = opcode;
                instruction.mnemonic = std::string(bytecodeTable::name(opcode));
                instruction.length = std::max<std::size_t>(1, length);
                instruction.operandText = decodeInstruction(code, index, constantPool, symbols);
                instructions.push_back(std::move(instruction));

                index += std::max<std::size_t>(1, length);
            }
            return instructions;
        }

        [[nodiscard]] std::string printImpl(const std::vector<instructionInfo> &instructions) {
            std::ostringstream stream;
            for (const auto &instruction: instructions) {
                stream << std::setw(4) << instruction.offset << ": " << instruction.mnemonic
                        << " (0x" << std::hex << static_cast<unsigned int>(instruction.opcode) << std::dec << ")";

                if (!instruction.operandText.empty()) {
                    stream << " " << instruction.operandText;
                }

                stream << '\n';
            }
            return stream.str();
        }
    }

    std::vector<instructionInfo> bytecodePrinter::decode(const std::vector<std::uint8_t> &code) {
        return bytecodePrinterDetail::decodeImpl(code, nullptr, nullptr);
    }

    std::vector<instructionInfo> bytecodePrinter::decode(const std::vector<std::uint8_t> &code,
                                                         const hotspot::constantPoolView &constantPool,
                                                         const hotspot::symbolTable &symbols) {
        return bytecodePrinterDetail::decodeImpl(code, &constantPool, &symbols);
    }

    std::string bytecodePrinter::print(const std::vector<std::uint8_t> &code) {
        return bytecodePrinterDetail::printImpl(decode(code));
    }

    std::string bytecodePrinter::print(const std::vector<std::uint8_t> &code,
                                       const hotspot::constantPoolView &constantPool,
                                       const hotspot::symbolTable &symbols) {
        return bytecodePrinterDetail::printImpl(decode(code, constantPool, symbols));
    }
}
