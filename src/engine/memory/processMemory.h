#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace splinter::engine::memory {
    class remoteProcess;

    class processMemory {
    public:
        explicit processMemory(const remoteProcess &process) noexcept;

        template<typename type>
        [[nodiscard]] type read(std::uint64_t address) const;

        [[nodiscard]] std::vector<std::byte> readBuffer(std::uint64_t address, std::size_t size) const;

        [[nodiscard]] std::string readCString(std::uint64_t address, std::size_t maxLength = 1024) const;

    private:
        const remoteProcess *process_ = nullptr;
    };

    template<typename type>
    type processMemory::read(std::uint64_t address) const {
        const auto buffer = readBuffer(address, sizeof(type));
        type value{};
        std::memcpy(&value, buffer.data(), sizeof(type));
        return value;
    }
}
