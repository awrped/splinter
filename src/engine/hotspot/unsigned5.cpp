#include "unsigned5.h"

#include <stdexcept>

namespace splinter::engine::hotspot::unsigned5 {
    bool hasNext(std::span<const std::byte> stream, std::size_t offset) noexcept {
        return offset < stream.size() && std::to_integer<std::uint8_t>(stream[offset]) != 0;
    }

    std::uint32_t readUint(std::span<const std::byte> stream, std::size_t &offset) {
        if (offset >= stream.size()) {
            throw std::runtime_error("UNSIGNED5 stream reached the end");
        }

        const std::size_t start = offset;
        const std::uint32_t first = std::to_integer<std::uint8_t>(stream[start]);
        if (first < excludedByte) {
            throw std::runtime_error("UNSIGNED5 stream used an excluded byte");
        }

        std::uint32_t sum = first - excludedByte;
        if (sum < lowByteCount) {
            offset = start + 1;
            return sum;
        }

        std::uint32_t shift = shiftPerByte;
        for (std::size_t index = 1; index < maxByteCount; ++index) {
            if (start + index >= stream.size()) {
                throw std::runtime_error("UNSIGNED5 stream ended mid value");
            }

            const std::uint32_t value = std::to_integer<std::uint8_t>(stream[start + index]);
            if (value < excludedByte) {
                throw std::runtime_error("UNSIGNED5 stream used an excluded byte");
            }

            sum += (value - excludedByte) << shift;
            if (value < excludedByte + lowByteCount || index == maxByteCount - 1) {
                offset = start + index + 1;
                return sum;
            }

            shift += shiftPerByte;
        }

        throw std::runtime_error("Invalid UNSIGNED5 value");
    }

    std::int32_t readInt(std::span<const std::byte> stream, std::size_t &offset) {
        const auto value = readUint(stream, offset);
        return static_cast<std::int32_t>((value >> 1) ^ -(static_cast<std::int32_t>(value & 1)));
    }
}
