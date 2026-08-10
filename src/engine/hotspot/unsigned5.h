#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace splinter::engine::hotspot::unsigned5 {
    // hotspot's modified pack200 UNSIGNED5 coding (utilities/unsigned5.hpp). byte
    // value 0 is excluded so a stream can be null terminated, which is why the low
    // byte count is 191 and not the 192 the unmodified pack200 coding uses
    inline constexpr std::uint32_t excludedByte = 1; // X
    inline constexpr std::uint32_t lowByteCount = 191; // L
    inline constexpr std::uint32_t shiftPerByte = 6; // lg_H
    inline constexpr std::size_t maxByteCount = 5;

    [[nodiscard]] bool hasNext(std::span<const std::byte> stream, std::size_t offset) noexcept;

    [[nodiscard]] std::uint32_t readUint(std::span<const std::byte> stream, std::size_t &offset);

    // zig zag encoded signed value, used by the compressed line number table
    [[nodiscard]] std::int32_t readInt(std::span<const std::byte> stream, std::size_t &offset);
}
