#include "testing.h"

#include "engine/classfile/accessFlags.h"
#include "engine/classfile/descriptorParser.h"
#include "engine/classfile/signatureParser.h"

namespace {
    using splinter::engine::classfile::accessFlags;
    using splinter::engine::classfile::descriptorParser;
    using splinter::engine::classfile::signatureParser;
}

TEST("field descriptors decode to java types") {
    CHECK_EQUAL(descriptorParser::parseField("B"), "byte");
    CHECK_EQUAL(descriptorParser::parseField("Z"), "boolean");
    CHECK_EQUAL(descriptorParser::parseField("J"), "long");
    CHECK_EQUAL(descriptorParser::parseField("V"), "void");
    CHECK_EQUAL(descriptorParser::parseField("Ljava/lang/String;"), "java.lang.String");
    CHECK_EQUAL(descriptorParser::parseField("[I"), "int[]");
    CHECK_EQUAL(descriptorParser::parseField("[[B"), "byte[][]");
    CHECK_EQUAL(descriptorParser::parseField("[Ljava/lang/Object;"), "java.lang.Object[]");
}

TEST("field descriptors survive malformed input") {
    CHECK_EQUAL(descriptorParser::parseField(""), "");
    // an unterminated class name stops at the end of the descriptor
    CHECK_EQUAL(descriptorParser::parseField("Ljava/lang/String"), "java.lang.String");
    // a dangling array marker keeps its brackets and drops the missing element type
    CHECK_EQUAL(descriptorParser::parseField("["), "[]");
    CHECK_EQUAL(descriptorParser::parseField("Q"), "Q");
}

TEST("parseNext advances past exactly one descriptor") {
    const std::string_view descriptor = "ILjava/lang/String;[J";
    std::size_t index = 0;

    CHECK_EQUAL(descriptorParser::parseNext(descriptor, index), "int");
    CHECK_EQUAL(index, std::size_t{1});
    CHECK_EQUAL(descriptorParser::parseNext(descriptor, index), "java.lang.String");
    CHECK_EQUAL(index, std::size_t{19});
    CHECK_EQUAL(descriptorParser::parseNext(descriptor, index), "long[]");
    CHECK_EQUAL(index, descriptor.size());
}

TEST("method descriptors decode to a readable signature") {
    CHECK_EQUAL(signatureParser::parseMethod("()V"), "() -> void");
    CHECK_EQUAL(signatureParser::parseMethod("(I)Z"), "(int) -> boolean");
    CHECK_EQUAL(signatureParser::parseMethod("(ILjava/lang/String;[J)Z"),
                "(int, java.lang.String, long[]) -> boolean");
    CHECK_EQUAL(signatureParser::parseMethod("([[Ljava/lang/String;)V"),
                "(java.lang.String[][]) -> void");
    CHECK_EQUAL(signatureParser::parseMethod("(Ljava/lang/Object;)Ljava/lang/Object;"),
                "(java.lang.Object) -> java.lang.Object");
}

TEST("method descriptors survive malformed input") {
    // anything that is not a descriptor comes back untouched instead of crashing
    CHECK_EQUAL(signatureParser::parseMethod(""), "");
    CHECK_EQUAL(signatureParser::parseMethod("notADescriptor"), "notADescriptor");
    CHECK_EQUAL(signatureParser::parseMethod("("), "() -> ");
    CHECK_EQUAL(signatureParser::parseMethod("(L"), "() -> ");
    CHECK_EQUAL(signatureParser::parseMethod("([["), "([][]) -> ");
}

TEST("access flags describe java modifiers") {
    CHECK_EQUAL(accessFlags(0x0009).describe(), "public static");
    CHECK_EQUAL(accessFlags(0x0401).describe(), "public abstract");
    CHECK_EQUAL(accessFlags(0x1042).describe(), "private synthetic bridge");
    CHECK_EQUAL(accessFlags(0).describe(), "");
    CHECK(accessFlags(0x0200).isInterface());
    CHECK(accessFlags(0x4000).isEnum());
    CHECK(accessFlags(0x0080).isVarargs());
}
