#pragma once

#include <cstdint>
#include <string>

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
        // 0x0020 is ACC_SUPER on a class and ACC_SYNCHRONIZED on a method
        [[nodiscard]] bool isSuper() const noexcept { return (bits_ & 0x0020u) != 0; }
        // 0x0040 is ACC_VOLATILE on a field and ACC_BRIDGE on a method
        [[nodiscard]] bool isVolatile() const noexcept { return (bits_ & 0x0040u) != 0; }
        [[nodiscard]] bool isBridge() const noexcept { return (bits_ & 0x0040u) != 0; }
        // 0x0080 is ACC_TRANSIENT on a field and ACC_VARARGS on a method
        [[nodiscard]] bool isTransient() const noexcept { return (bits_ & 0x0080u) != 0; }
        [[nodiscard]] bool isVarargs() const noexcept { return (bits_ & 0x0080u) != 0; }
        [[nodiscard]] bool isNative() const noexcept { return (bits_ & 0x0100u) != 0; }
        [[nodiscard]] bool isInterface() const noexcept { return (bits_ & 0x0200u) != 0; }
        [[nodiscard]] bool isAbstract() const noexcept { return (bits_ & 0x0400u) != 0; }
        [[nodiscard]] bool isStrict() const noexcept { return (bits_ & 0x0800u) != 0; }
        [[nodiscard]] bool isSynthetic() const noexcept { return (bits_ & 0x1000u) != 0; }
        [[nodiscard]] bool isAnnotation() const noexcept { return (bits_ & 0x2000u) != 0; }
        [[nodiscard]] bool isEnum() const noexcept { return (bits_ & 0x4000u) != 0; }

        // java source order, the shared bits are rendered with their method meaning
        [[nodiscard]] std::string describe() const {
            std::string result;
            const auto append = [&result](const char *modifier) {
                if (!result.empty()) {
                    result += ' ';
                }
                result += modifier;
            };

            if (isPublic()) append("public");
            if (isProtected()) append("protected");
            if (isPrivate()) append("private");
            if (isAbstract()) append("abstract");
            if (isStatic()) append("static");
            if (isFinal()) append("final");
            if (isSynchronized()) append("synchronized");
            if (isNative()) append("native");
            if (isStrict()) append("strictfp");
            if (isSynthetic()) append("synthetic");
            if (isBridge()) append("bridge");
            if (isVarargs()) append("varargs");
            return result;
        }

    private:
        std::uint32_t bits_ = 0;
    };
}
