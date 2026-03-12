#pragma once

#include <cstdint>

namespace splinter::engine::classfile {
    class accessFlags {
    public:
        explicit accessFlags(std::uint32_t bits = 0) noexcept : bits_(bits) {
        }

        [[nodiscard]] std::uint32_t bits() const noexcept { return bits_; }
        [[nodiscard]] bool isPublic() const noexcept { return (bits_ & 0x0001u) != 0; }
        [[nodiscard]] bool isPrivate() const noexcept { return (bits_ & 0x0002u) != 0; }
        [[nodiscard]] bool isProtected() const noexcept { return (bits_ & 0x0004u) != 0; }
        [[nodiscard]] bool isStatic() const noexcept { return (bits_ & 0x0008u) != 0; }
        [[nodiscard]] bool isFinal() const noexcept { return (bits_ & 0x0010u) != 0; }
        [[nodiscard]] bool isSynchronized() const noexcept { return (bits_ & 0x0020u) != 0; }
        [[nodiscard]] bool isNative() const noexcept { return (bits_ & 0x0100u) != 0; }
        [[nodiscard]] bool isAbstract() const noexcept { return (bits_ & 0x0400u) != 0; }

    private:
        std::uint32_t bits_ = 0;
    };
}
