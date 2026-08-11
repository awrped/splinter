#include "testing.h"

#include "engine/hotspot/unsigned5.h"

#include <cstddef>
#include <vector>

namespace {
    using namespace splinter::engine::hotspot;

    std::vector<std::byte> bytes(std::initializer_list<int> values) {
        std::vector<std::byte> result;
        result.reserve(values.size());
        for (const auto value: values) {
            result.push_back(static_cast<std::byte>(value));
        }
        return result;
    }

    std::uint32_t readOne(const std::vector<std::byte> &stream) {
        std::size_t offset = 0;
        return unsigned5::readUint(stream, offset);
    }

    std::int32_t readOneInt(const std::vector<std::byte> &stream) {
        std::size_t offset = 0;
        return unsigned5::readInt(stream, offset);
    }
}

TEST("unsigned5 decodes single byte values") {
    CHECK_EQUAL(readOne(bytes({0x01})), 0u);
    CHECK_EQUAL(readOne(bytes({0x02})), 1u);
    // 0xBF is the largest single byte, every byte carries a bias of one
    CHECK_EQUAL(readOne(bytes({0xBF})), 190u);
}

TEST("unsigned5 uses 191 as the low byte count") {
    // hotspot excludes byte value 0 so L is 191, not the 192 of plain pack200.
    // 0xC0 therefore starts a multi byte value whose first continuation adds zero
    CHECK_EQUAL(readOne(bytes({0xC0, 0x01})), 191u);
    CHECK_EQUAL(readOne(bytes({0xC0, 0x02})), 255u);
    CHECK_EQUAL(readOne(bytes({0xC1, 0x02})), 256u);
}

TEST("unsigned5 advances the offset by the bytes it consumed") {
    const auto stream = bytes({0x05, 0xC0, 0x02, 0x03});
    std::size_t offset = 0;

    CHECK_EQUAL(unsigned5::readUint(stream, offset), 4u);
    CHECK_EQUAL(offset, std::size_t{1});
    CHECK_EQUAL(unsigned5::readUint(stream, offset), 255u);
    CHECK_EQUAL(offset, std::size_t{3});
    CHECK_EQUAL(unsigned5::readUint(stream, offset), 2u);
    CHECK_EQUAL(offset, std::size_t{4});
    CHECK(!unsigned5::hasNext(stream, offset));
}

TEST("unsigned5 spans the full five byte range") {
    // every continuation byte contributes six more bits
    const auto stream = bytes({0xC0, 0xC0, 0xC0, 0xC0, 0x02});
    std::size_t offset = 0;
    const auto value = unsigned5::readUint(stream, offset);

    CHECK_EQUAL(offset, std::size_t{5});
    CHECK_EQUAL(value, 191u + (191u << 6) + (191u << 12) + (191u << 18) + (1u << 24));
}

TEST("unsigned5 reads zig zag signed values") {
    CHECK_EQUAL(readOneInt(bytes({0x01})), 0);
    CHECK_EQUAL(readOneInt(bytes({0x02})), -1);
    CHECK_EQUAL(readOneInt(bytes({0x03})), 1);
    CHECK_EQUAL(readOneInt(bytes({0x05})), 2);
    CHECK_EQUAL(readOneInt(bytes({0x04})), -2);
}

TEST("unsigned5 treats a zero byte as the end of the stream") {
    const auto stream = bytes({0x05, 0x00, 0x07});

    CHECK(unsigned5::hasNext(stream, 0));
    CHECK(!unsigned5::hasNext(stream, 1));
    CHECK(!unsigned5::hasNext(stream, stream.size()));
}

TEST("unsigned5 rejects excluded and truncated input") {
    std::size_t offset = 0;
    const auto excluded = bytes({0x00});
    CHECK_THROWS(unsigned5::readUint(excluded, offset));

    offset = 0;
    const auto excludedContinuation = bytes({0xC0, 0x00});
    CHECK_THROWS(unsigned5::readUint(excludedContinuation, offset));

    offset = 0;
    const auto truncated = bytes({0xC0});
    CHECK_THROWS(unsigned5::readUint(truncated, offset));

    offset = 0;
    const std::vector<std::byte> empty;
    CHECK_THROWS(unsigned5::readUint(empty, offset));
}
