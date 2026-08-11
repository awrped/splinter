#pragma once

#include "moduleMap.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace splinter::engine::memory {
    struct processInfo {
        std::uint32_t pid = 0;
        std::wstring name;
        std::wstring path;
    };

    struct attachOptions {
        // 0 searches by image name instead of attaching to a specific process
        std::uint32_t pid = 0;
        // empty falls back to javaw.exe and java.exe
        std::vector<std::wstring> processNames;
    };

    struct processCandidate {
        std::uint32_t pid = 0;
        std::wstring name;
        bool readable = false;
        bool hasJvm = false;
    };

    class remoteProcess {
    public:
        remoteProcess();

        ~remoteProcess();

        remoteProcess(const remoteProcess &) = delete;

        remoteProcess &operator=(const remoteProcess &) = delete;

        remoteProcess(remoteProcess &&other) noexcept;

        remoteProcess &operator=(remoteProcess &&other) noexcept;

        bool attach(const attachOptions &options = {});

        bool attachToJavaw();

        void close() noexcept;

        [[nodiscard]] bool isOpen() const noexcept;

        [[nodiscard]] const std::string &lastError() const noexcept;

        [[nodiscard]] const processInfo &process() const noexcept;

        [[nodiscard]] const moduleMap &modules() const noexcept;

        [[nodiscard]] const moduleInfo *jvmModule() const noexcept;

        [[nodiscard]] std::vector<std::byte> read(std::uint64_t address, std::size_t size) const;

        [[nodiscard]] std::string readCString(std::uint64_t address, std::size_t maxLength = 1024) const;

        [[nodiscard]] std::uint64_t resolveExport(std::wstring_view moduleName, std::string_view exportName) const;

        [[nodiscard]] std::string describeTarget() const;

    private:
        class implementation;
        std::unique_ptr<implementation> implementation_;
    };

    std::string narrow(std::wstring_view value);

    std::wstring widen(std::string_view value);

    // every java looking process on the machine, so the caller can report why none
    // of them could be attached to
    [[nodiscard]] std::vector<processCandidate> enumerateJavaProcesses();
}
