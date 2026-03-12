#include "remoteProcess.h"

#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cwctype>
#include <format>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace splinter::engine::memory {
    namespace detail {
        class uniqueHandle {
        public:
            uniqueHandle() noexcept = default;

            explicit uniqueHandle(HANDLE value) noexcept : value_(value) {
            }

            ~uniqueHandle() {
                reset();
            }

            uniqueHandle(const uniqueHandle &) = delete;

            uniqueHandle &operator=(const uniqueHandle &) = delete;

            uniqueHandle(uniqueHandle &&other) noexcept : value_(other.release()) {
            }

            uniqueHandle &operator=(uniqueHandle &&other) noexcept {
                if (this != &other) {
                    reset(other.release());
                }
                return *this;
            }

            [[nodiscard]] HANDLE get() const noexcept {
                return value_;
            }

            [[nodiscard]] explicit operator bool() const noexcept {
                return value_ != nullptr && value_ != INVALID_HANDLE_VALUE;
            }

            [[nodiscard]] HANDLE release() noexcept {
                const HANDLE released = value_;
                value_ = nullptr;
                return released;
            }

            void reset(HANDLE value = nullptr) noexcept {
                if (value_ != nullptr && value_ != INVALID_HANDLE_VALUE) {
                    CloseHandle(value_);
                }
                value_ = value;
            }

        private:
            HANDLE value_ = nullptr;
        };

        class uniqueModule {
        public:
            uniqueModule() noexcept = default;

            explicit uniqueModule(HMODULE value) noexcept : value_(value) {
            }

            ~uniqueModule() {
                reset();
            }

            uniqueModule(const uniqueModule &) = delete;

            uniqueModule &operator=(const uniqueModule &) = delete;

            uniqueModule(uniqueModule &&other) noexcept : value_(other.release()) {
            }

            uniqueModule &operator=(uniqueModule &&other) noexcept {
                if (this != &other) {
                    reset(other.release());
                }
                return *this;
            }

            [[nodiscard]] HMODULE get() const noexcept {
                return value_;
            }

            [[nodiscard]] explicit operator bool() const noexcept {
                return value_ != nullptr;
            }

            [[nodiscard]] HMODULE release() noexcept {
                const HMODULE released = value_;
                value_ = nullptr;
                return released;
            }

            void reset(HMODULE value = nullptr) noexcept {
                if (value_ != nullptr) {
                    FreeLibrary(value_);
                }
                value_ = value;
            }

        private:
            HMODULE value_ = nullptr;
        };

        [[nodiscard]] std::string lastErrorMessage(std::string_view context) {
            const DWORD error = GetLastError();
            LPWSTR buffer = nullptr;
            const DWORD length = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                0,
                reinterpret_cast<LPWSTR>(&buffer),
                0,
                nullptr);

            std::wstring message;
            if (length > 0 && buffer != nullptr) {
                message.assign(buffer, length);
                LocalFree(buffer);
            }

            while (!message.empty() && (message.back() == L'\n' || message.back() == L'\r')) {
                message.pop_back();
            }

            return std::format("{} (Win32 error {}: {})", context, error, narrow(message));
        }

        [[nodiscard]] bool equalsIgnoreCase(std::wstring_view lhs, std::wstring_view rhs) {
            if (lhs.size() != rhs.size()) {
                return false;
            }

            for (std::size_t index = 0; index < lhs.size(); ++index) {
                if (::towlower(lhs[index]) != ::towlower(rhs[index])) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::wstring queryProcessPath(HANDLE processHandle) {
            std::wstring buffer(MAX_PATH, L'\0');
            DWORD length = static_cast<DWORD>(buffer.size());
            if (!QueryFullProcessImageNameW(processHandle, 0, buffer.data(), &length)) {
                return {};
            }

            buffer.resize(length);
            return buffer;
        }
    }

    std::string narrow(std::wstring_view value) {
        if (value.empty()) {
            return {};
        }

        const int required = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0,
                                                 nullptr, nullptr);
        std::string result(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), required, nullptr,
                            nullptr);
        return result;
    }

    class remoteProcess::implementation {
    public:
        static constexpr std::wstring_view targetProcessName = L"javaw.exe";
        static constexpr std::wstring_view jvmModuleName = L"jvm.dll";

        bool attachToJavaw() {
            close();

            detail::uniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
            if (!snapshot) {
                lastError_ = detail::lastErrorMessage("CreateToolhelp32Snapshot failed");
                return false;
            }

            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (!Process32FirstW(snapshot.get(), &entry)) {
                lastError_ = detail::lastErrorMessage("Process32FirstW failed");
                return false;
            }

            do {
                if (detail::equalsIgnoreCase(entry.szExeFile, targetProcessName) && tryAttachProcess(entry)) {
                    return true;
                }
            } while (Process32NextW(snapshot.get(), &entry));

            lastError_ = "Unable to find a readable javaw.exe process with jvm.dll loaded";
            return false;
        }

        void close() noexcept {
            processHandle_.reset();
            process_ = {};
            modules_.clear();
            lastError_.clear();
        }

        [[nodiscard]] bool isOpen() const noexcept {
            return static_cast<bool>(processHandle_);
        }

        [[nodiscard]] const moduleInfo *jvmModule() const noexcept {
            return modules_.findByName(jvmModuleName);
        }

        [[nodiscard]] std::vector<std::byte> read(std::uint64_t address, std::size_t size) const {
            std::vector<std::byte> buffer(size);
            SIZE_T bytesRead = 0;
            if (!ReadProcessMemory(processHandle_.get(),
                                   reinterpret_cast<LPCVOID>(address),
                                   buffer.data(),
                                   size,
                                   &bytesRead) || bytesRead != size) {
                throw std::runtime_error(
                    detail::lastErrorMessage(std::format("ReadProcessMemory failed at 0x{:X}", address)));
            }
            return buffer;
        }

        [[nodiscard]] std::string readCString(std::uint64_t address, std::size_t maxLength) const {
            if (address == 0) {
                return {};
            }

            std::string result;
            result.reserve(std::min<std::size_t>(64, maxLength));
            for (std::size_t index = 0; index < maxLength; ++index) {
                const char value = static_cast<char>(read(address + index, 1)[0]);
                if (value == '\0') {
                    return result;
                }
                result.push_back(value);
            }

            throw std::runtime_error(std::format("Remote string at 0x{:X} exceeded {} bytes", address, maxLength));
        }

        [[nodiscard]] std::uint64_t resolveExport(std::wstring_view moduleName, std::string_view exportName) const {
            const moduleInfo *module = modules_.findByName(moduleName);
            if (module == nullptr) {
                throw std::runtime_error(std::format("Module {} is not loaded in target process", narrow(moduleName)));
            }

            detail::uniqueModule
                    localModule(LoadLibraryExW(module->path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
            if (!localModule) {
                throw std::runtime_error(
                    detail::lastErrorMessage(std::format("LoadLibraryExW failed for {}", narrow(module->path))));
            }

            FARPROC symbol = GetProcAddress(localModule.get(), std::string(exportName).c_str());
            if (symbol == nullptr) {
                throw std::runtime_error(std::format("Export {} was not found in {}", exportName, narrow(moduleName)));
            }

            const auto localBase = reinterpret_cast<std::uintptr_t>(localModule.get());
            const auto localSymbol = reinterpret_cast<std::uintptr_t>(symbol);
            return module->base + (localSymbol - localBase);
        }

        [[nodiscard]] std::string describeTarget() const {
            const moduleInfo *jvm = jvmModule();
            std::ostringstream stream;
            stream << "pid=" << process_.pid
                    << " process=" << narrow(process_.name)
                    << " processPath=" << narrow(process_.path);
            if (jvm != nullptr) {
                stream << " jvmPath=" << narrow(jvm->path) << " jvmBase=0x" << std::hex << jvm->base;
            }
            return stream.str();
        }

        processInfo process_{};
        moduleMap modules_{};
        std::string lastError_;
        detail::uniqueHandle processHandle_{};

    private:
        bool tryAttachProcess(const PROCESSENTRY32W &entry) {
            detail::uniqueHandle processHandle(
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID));
            if (!processHandle) {
                return false;
            }

            moduleMap discoveredModules;
            if (!enumerateModules(entry.th32ProcessID, discoveredModules) ||
                discoveredModules.findByName(jvmModuleName) == nullptr) {
                return false;
            }

            process_ = processInfo{entry.th32ProcessID, entry.szExeFile, detail::queryProcessPath(processHandle.get())};
            modules_ = std::move(discoveredModules);
            processHandle_.reset(processHandle.release());
            lastError_.clear();
            return true;
        }

        [[nodiscard]] bool enumerateModules(std::uint32_t pid, moduleMap &outModules) const {
            detail::uniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
            if (!snapshot) {
                return false;
            }

            MODULEENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (!Module32FirstW(snapshot.get(), &entry)) {
                return false;
            }

            do {
                outModules.add(moduleInfo{
                    entry.szModule,
                    entry.szExePath,
                    reinterpret_cast<std::uint64_t>(entry.modBaseAddr),
                    static_cast<std::uint64_t>(entry.modBaseSize)
                });
            } while (Module32NextW(snapshot.get(), &entry));

            return true;
        }
    };

    remoteProcess::remoteProcess() : implementation_(new implementation()) {
    }

    remoteProcess::~remoteProcess() {
        delete implementation_;
    }

    remoteProcess::remoteProcess(remoteProcess &&other) noexcept
        : implementation_(std::exchange(other.implementation_, nullptr)) {
    }

    remoteProcess &remoteProcess::operator=(remoteProcess &&other) noexcept {
        if (this != &other) {
            delete implementation_;
            implementation_ = std::exchange(other.implementation_, nullptr);
        }
        return *this;
    }

    bool remoteProcess::attachToJavaw() {
        return implementation_ != nullptr && implementation_->attachToJavaw();
    }

    void remoteProcess::close() noexcept {
        if (implementation_ != nullptr) {
            implementation_->close();
        }
    }

    bool remoteProcess::isOpen() const noexcept {
        return implementation_ != nullptr && implementation_->isOpen();
    }

    const std::string &remoteProcess::lastError() const noexcept {
        static const std::string missing = "remoteProcess implementation missing";
        return implementation_ != nullptr ? implementation_->lastError_ : missing;
    }

    const processInfo &remoteProcess::process() const noexcept {
        static const processInfo empty{};
        return implementation_ != nullptr ? implementation_->process_ : empty;
    }

    const moduleMap &remoteProcess::modules() const noexcept {
        static const moduleMap empty{};
        return implementation_ != nullptr ? implementation_->modules_ : empty;
    }

    const moduleInfo *remoteProcess::jvmModule() const noexcept {
        return implementation_ != nullptr ? implementation_->jvmModule() : nullptr;
    }

    std::vector<std::byte> remoteProcess::read(std::uint64_t address, std::size_t size) const {
        if (implementation_ == nullptr) {
            throw std::runtime_error("remoteProcess implementation missing");
        }
        return implementation_->read(address, size);
    }

    std::string remoteProcess::readCString(std::uint64_t address, std::size_t maxLength) const {
        if (implementation_ == nullptr) {
            throw std::runtime_error("remoteProcess implementation missing");
        }
        return implementation_->readCString(address, maxLength);
    }

    std::uint64_t remoteProcess::resolveExport(std::wstring_view moduleName, std::string_view exportName) const {
        if (implementation_ == nullptr) {
            throw std::runtime_error("remoteProcess implementation missing");
        }
        return implementation_->resolveExport(moduleName, exportName);
    }

    std::string remoteProcess::describeTarget() const {
        return implementation_ != nullptr ? implementation_->describeTarget() : "remoteProcess implementation missing";
    }
}
