#include "processMemory.h"

#include "remoteProcess.h"

#include <cstring>

namespace splinter::engine::memory {
    processMemory::processMemory(const remoteProcess &process) noexcept : process_(&process) {
    }

    std::vector<std::byte> processMemory::readBuffer(std::uint64_t address, std::size_t size) const {
        return process_->read(address, size);
    }

    std::string processMemory::readCString(std::uint64_t address, std::size_t maxLength) const {
        return process_->readCString(address, maxLength);
    }
}
