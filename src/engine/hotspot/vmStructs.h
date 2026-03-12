#pragma once

#include "../memory/processMemory.h"
#include "../memory/remoteProcess.h"
#include "vmConstants.h"
#include "vmTypes.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace splinter::engine::hotspot {
    struct vmStructEntry {
        std::string typeName;
        std::string fieldName;
        std::string typeString;
        bool isStatic = false;
        std::uint64_t offset = 0;
        std::uint64_t address = 0;
    };

    struct exportedArrayLayout {
        std::uint64_t address = 0;
        std::uint64_t stride = 0;
    };

    struct vmStructLayout {
        exportedArrayLayout array;
        std::uint64_t typeNameOffset = 0;
        std::uint64_t fieldNameOffset = 0;
        std::uint64_t typeStringOffset = 0;
        std::uint64_t isStaticOffset = 0;
        std::uint64_t offsetOffset = 0;
        std::uint64_t addressOffset = 0;
    };

    struct vmTypeLayout {
        exportedArrayLayout array;
        std::uint64_t typeNameOffset = 0;
        std::uint64_t superNameOffset = 0;
        std::uint64_t isOopOffset = 0;
        std::uint64_t isIntegerOffset = 0;
        std::uint64_t isUnsignedOffset = 0;
        std::uint64_t sizeOffset = 0;
    };

    struct vmConstantLayout {
        exportedArrayLayout array;
        std::uint64_t nameOffset = 0;
        std::uint64_t valueOffset = 0;
    };

    class vmStructs {
    public:
        bool refresh(const memory::remoteProcess &process);

        [[nodiscard]] const std::string &lastError() const noexcept;

        [[nodiscard]] const std::vector<vmStructEntry> &fields() const noexcept;

        [[nodiscard]] const vmTypes &types() const noexcept;

        [[nodiscard]] const vmConstants &constants() const noexcept;

        [[nodiscard]] const vmStructEntry *findField(std::string_view typeName,
                                                     std::string_view fieldName) const noexcept;

        [[nodiscard]] const vmTypeInfo *findType(std::string_view typeName) const noexcept;

        [[nodiscard]] const vmStructLayout &structLayout() const noexcept;

        [[nodiscard]] const vmTypeLayout &typeLayout() const noexcept;

        [[nodiscard]] const vmConstantLayout &intConstantLayout() const noexcept;

        [[nodiscard]] const vmConstantLayout &longConstantLayout() const noexcept;

    private:
        [[nodiscard]] bool loadLayouts(const memory::remoteProcess &process, const memory::processMemory &memory);

        [[nodiscard]] bool loadFields(const memory::processMemory &memory);

        [[nodiscard]] bool loadTypes(const memory::processMemory &memory);

        [[nodiscard]] bool loadIntConstants(const memory::processMemory &memory);

        [[nodiscard]] bool loadLongConstants(const memory::processMemory &memory);

        [[nodiscard]] std::optional<std::uint64_t> readExportValue(
            const memory::remoteProcess &process,
            const memory::processMemory &memory,
            std::string_view exportName) const;

        [[nodiscard]] std::string readRemoteString(const memory::processMemory &memory, std::uint64_t address) const;

        template<typename type>
        [[nodiscard]] type decodeScalar(const std::vector<std::byte> &buffer, std::uint64_t offset) const;

        std::string lastError_;
        std::vector<vmStructEntry> fields_;
        vmTypes types_;
        vmConstants constants_;
        vmStructLayout structLayout_{};
        vmTypeLayout typeLayout_{};
        vmConstantLayout intConstantLayout_{};
        vmConstantLayout longConstantLayout_{};
    };

    template<typename type>
    type vmStructs::decodeScalar(const std::vector<std::byte> &buffer, std::uint64_t offset) const {
        if (offset + sizeof(type) > buffer.size()) {
            throw std::runtime_error("vmStructs decode overflow");
        }

        type value{};
        std::memcpy(&value, buffer.data() + offset, sizeof(type));
        return value;
    }
}
