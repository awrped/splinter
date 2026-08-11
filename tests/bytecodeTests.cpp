#include "testing.h"

#include "engine/bytecode/bytecodePrinter.h"
#include "engine/bytecode/bytecodeTable.h"

#include <cstdint>
#include <vector>

namespace {
    using splinter::engine::bytecode::bytecodePrinter;
    using splinter::engine::bytecode::bytecodeTable;

    std::vector<std::uint8_t> code(std::initializer_list<int> values) {
        std::vector<std::uint8_t> result;
        result.reserve(values.size());
        for (const auto value: values) {
            result.push_back(static_cast<std::uint8_t>(value));
        }
        return result;
    }

    std::vector<std::uint8_t> padded(std::vector<std::uint8_t> prefix, std::size_t size) {
        prefix.resize(size, 0);
        return prefix;
    }
}

TEST("instruction lengths follow the opcode table") {
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0x00}), 0), std::size_t{1}); // nop
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0x10, 0x05}), 0), std::size_t{2}); // bipush
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0x11, 0x00, 0x05}), 0), std::size_t{3}); // sipush
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0x84, 0x01, 0x01}), 0), std::size_t{3}); // iinc
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0xb6, 0x00, 0x01}), 0), std::size_t{3}); // invokevirtual
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xb9}), 5), 0), std::size_t{5}); // invokeinterface
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xc8}), 5), 0), std::size_t{5}); // goto_w
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xdd}), 4), 0), std::size_t{4}); // fast_iaccess_0
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xc5}), 4), 0), std::size_t{4}); // multianewarray
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({}), 0), std::size_t{0});
}

TEST("wide takes its length from the widened opcode") {
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xc4, 0x15}), 4), 0), std::size_t{4}); // wide iload
    CHECK_EQUAL(bytecodePrinter::instructionLength(padded(code({0xc4, 0x84}), 6), 0), std::size_t{6}); // wide iinc
    // a wide with nothing after it still consumes at least one byte
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0xc4}), 0), std::size_t{1});
}

TEST("tableswitch pads to the next four byte boundary") {
    // opcode at 0, three pad bytes, default, low = 0, high = 1, then two targets
    auto tableSwitch = padded(code({0xaa}), 24);
    tableSwitch[15] = 0x01; // high
    CHECK_EQUAL(bytecodePrinter::instructionLength(tableSwitch, 0), std::size_t{24});

    // the same instruction one byte further along needs one pad byte less
    auto shifted = padded(code({0x00, 0xaa}), 24);
    shifted[15] = 0x01;
    CHECK_EQUAL(bytecodePrinter::instructionLength(shifted, 1), std::size_t{23});
}

TEST("lookupswitch length follows the pair count") {
    // opcode at 0, three pad bytes, default, npairs = 2, then two eight byte pairs
    auto lookupSwitch = padded(code({0xab}), 28);
    lookupSwitch[11] = 0x02; // npairs
    CHECK_EQUAL(bytecodePrinter::instructionLength(lookupSwitch, 0), std::size_t{28});
}

TEST("truncated switches do not run past the end of the code") {
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0xaa}), 0), std::size_t{1});
    CHECK_EQUAL(bytecodePrinter::instructionLength(code({0xab, 0x00}), 0), std::size_t{2});

    // a negative pair count must not turn into a huge length
    auto negativePairs = padded(code({0xab}), 12);
    negativePairs[8] = 0xff;
    negativePairs[9] = 0xff;
    negativePairs[10] = 0xff;
    negativePairs[11] = 0xff;
    CHECK(bytecodePrinter::instructionLength(negativePairs, 0) <= negativePairs.size());
}

TEST("decoding a truncated operand does not read past the code") {
    // the operand bytes are missing entirely, every accessor has to fall back to zero
    for (const auto opcode: {0x10, 0x11, 0x12, 0x13, 0x84, 0xb6, 0xb9, 0xba, 0xbc, 0xc5, 0xe1}) {
        const auto instructions = bytecodePrinter::decode(code({opcode}));
        CHECK_EQUAL(instructions.size(), std::size_t{1});
        CHECK_EQUAL(instructions.front().opcode, static_cast<std::uint8_t>(opcode));
    }
}

TEST("decoding walks every instruction exactly once") {
    const auto instructions = bytecodePrinter::decode(code({
        0x2a, // aload_0
        0x10, 0x2a, // bipush 42
        0x84, 0x01, 0xff, // iinc 1 -1
        0xb1 // return
    }));

    CHECK_EQUAL(instructions.size(), std::size_t{4});
    CHECK_EQUAL(instructions[0].mnemonic, "aload_0");
    CHECK_EQUAL(instructions[1].mnemonic, "bipush");
    CHECK_EQUAL(instructions[1].operandText, "value=42");
    CHECK_EQUAL(instructions[2].mnemonic, "iinc");
    CHECK_EQUAL(instructions[2].operandText, "local=1 delta=-1");
    CHECK_EQUAL(instructions[3].mnemonic, "return");
    CHECK_EQUAL(instructions[3].offset, std::size_t{6});
}

TEST("branch targets are relative to the opcode") {
    const auto instructions = bytecodePrinter::decode(code({
        0x00, // nop
        0xa7, 0xff, 0xfe // goto -2
    }));

    CHECK_EQUAL(instructions.size(), std::size_t{2});
    CHECK_EQUAL(instructions[1].operandText, "target=-1");
}

TEST("decoding an empty method yields nothing") {
    CHECK(bytecodePrinter::decode(code({})).empty());
    CHECK(bytecodePrinter::print(code({})).empty());
}

TEST("the opcode table covers runtime bytecodes") {
    CHECK_EQUAL(bytecodeTable::name(0x00), "nop");
    CHECK_EQUAL(bytecodeTable::name(0xb6), "invokevirtual");
    CHECK_EQUAL(bytecodeTable::name(0xba), "invokedynamic");
    // hotspot only bytecodes, these never appear in a classfile
    CHECK_EQUAL(bytecodeTable::name(0xd0), "fast_igetfield");
    CHECK_EQUAL(bytecodeTable::name(0xdd), "fast_iaccess_0");
    CHECK_EQUAL(bytecodeTable::name(0xe6), "fast_aldc");
    CHECK_EQUAL(bytecodeTable::name(0xe9), "invokehandle");
    CHECK_EQUAL(bytecodeTable::name(0xff), "unknown");
}
