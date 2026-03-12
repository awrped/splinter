#include "vmStructs.h"

namespace splinter::engine::hotspot {
    bool vmStructs::refresh(const memory::remoteProcess &process) {
        lastError_.clear();
        fields_.clear();
        types_.clear();
        constants_.clear();

        try {
            memory::processMemory memory(process);
            return loadLayouts(process, memory) && loadFields(memory) && loadTypes(memory) && loadIntConstants(memory)
                   &&
                   loadLongConstants(memory);
        } catch (const std::exception &exception) {
            lastError_ = exception.what();
            return false;
        }
    }

    const std::string &vmStructs::lastError() const noexcept {
        return lastError_;
    }

    const std::vector<vmStructEntry> &vmStructs::fields() const noexcept {
        return fields_;
    }

    const vmTypes &vmStructs::types() const noexcept {
        return types_;
    }

    const vmConstants &vmStructs::constants() const noexcept {
        return constants_;
    }

    const vmStructEntry *vmStructs::findField(std::string_view typeName, std::string_view fieldName) const noexcept {
        for (const auto &entry: fields_) {
            if (entry.typeName == typeName && entry.fieldName == fieldName) {
                return &entry;
            }
        }
        return nullptr;
    }

    const vmTypeInfo *vmStructs::findType(std::string_view typeName) const noexcept {
        return types_.find(typeName);
    }

    const vmStructLayout &vmStructs::structLayout() const noexcept {
        return structLayout_;
    }

    const vmTypeLayout &vmStructs::typeLayout() const noexcept {
        return typeLayout_;
    }

    const vmConstantLayout &vmStructs::intConstantLayout() const noexcept {
        return intConstantLayout_;
    }

    const vmConstantLayout &vmStructs::longConstantLayout() const noexcept {
        return longConstantLayout_;
    }

    bool vmStructs::loadLayouts(const memory::remoteProcess &process, const memory::processMemory &memory) {
        const auto fieldArray = readExportValue(process, memory, "gHotSpotVMStructs");
        const auto fieldStride = readExportValue(process, memory, "gHotSpotVMStructEntryArrayStride");
        const auto typeArray = readExportValue(process, memory, "gHotSpotVMTypes");
        const auto typeStride = readExportValue(process, memory, "gHotSpotVMTypeEntryArrayStride");
        const auto intArray = readExportValue(process, memory, "gHotSpotVMIntConstants");
        const auto intStride = readExportValue(process, memory, "gHotSpotVMIntConstantEntryArrayStride");
        const auto longArray = readExportValue(process, memory, "gHotSpotVMLongConstants");
        const auto longStride = readExportValue(process, memory, "gHotSpotVMLongConstantEntryArrayStride");

        if (!fieldArray || !fieldStride || !typeArray || !typeStride || !intArray || !intStride || !longArray || !
            longStride) {
            lastError_ = "Missing required VMStruct exports";
            return false;
        }

        structLayout_.array = {*fieldArray, *fieldStride};
        typeLayout_.array = {*typeArray, *typeStride};
        intConstantLayout_.array = {*intArray, *intStride};
        longConstantLayout_.array = {*longArray, *longStride};

        structLayout_.typeNameOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryTypeNameOffset");
        structLayout_.fieldNameOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryFieldNameOffset");
        structLayout_.typeStringOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryTypeStringOffset");
        structLayout_.isStaticOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryIsStaticOffset");
        structLayout_.offsetOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryOffsetOffset");
        structLayout_.addressOffset = *readExportValue(process, memory, "gHotSpotVMStructEntryAddressOffset");

        typeLayout_.typeNameOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntryTypeNameOffset");
        typeLayout_.superNameOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntrySuperclassNameOffset");
        typeLayout_.isOopOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntryIsOopTypeOffset");
        typeLayout_.isIntegerOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntryIsIntegerTypeOffset");
        typeLayout_.isUnsignedOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntryIsUnsignedOffset");
        typeLayout_.sizeOffset = *readExportValue(process, memory, "gHotSpotVMTypeEntrySizeOffset");

        intConstantLayout_.nameOffset = *readExportValue(process, memory, "gHotSpotVMIntConstantEntryNameOffset");
        intConstantLayout_.valueOffset = *readExportValue(process, memory, "gHotSpotVMIntConstantEntryValueOffset");
        longConstantLayout_.nameOffset = *readExportValue(process, memory, "gHotSpotVMLongConstantEntryNameOffset");
        longConstantLayout_.valueOffset = *readExportValue(process, memory, "gHotSpotVMLongConstantEntryValueOffset");
        return true;
    }

    bool vmStructs::loadFields(const memory::processMemory &memory) {
        for (std::uint64_t index = 0;; ++index) {
            const auto buffer = memory.readBuffer(structLayout_.array.address + index * structLayout_.array.stride,
                                                  static_cast<std::size_t>(structLayout_.array.stride));
            const std::uint64_t fieldNameAddress = decodeScalar<std::uint64_t>(buffer, structLayout_.fieldNameOffset);
            if (fieldNameAddress == 0) {
                break;
            }

            fields_.push_back(vmStructEntry{
                readRemoteString(memory, decodeScalar<std::uint64_t>(buffer, structLayout_.typeNameOffset)),
                readRemoteString(memory, fieldNameAddress),
                readRemoteString(memory, decodeScalar<std::uint64_t>(buffer, structLayout_.typeStringOffset)),
                decodeScalar<std::int32_t>(buffer, structLayout_.isStaticOffset) != 0,
                decodeScalar<std::uint64_t>(buffer, structLayout_.offsetOffset),
                decodeScalar<std::uint64_t>(buffer, structLayout_.addressOffset)
            });
        }
        return true;
    }

    bool vmStructs::loadTypes(const memory::processMemory &memory) {
        for (std::uint64_t index = 0;; ++index) {
            const auto buffer = memory.readBuffer(typeLayout_.array.address + index * typeLayout_.array.stride,
                                                  static_cast<std::size_t>(typeLayout_.array.stride));
            const std::uint64_t typeNameAddress = decodeScalar<std::uint64_t>(buffer, typeLayout_.typeNameOffset);
            if (typeNameAddress == 0) {
                break;
            }

            types_.add(vmTypeInfo{
                readRemoteString(memory, typeNameAddress),
                readRemoteString(memory, decodeScalar<std::uint64_t>(buffer, typeLayout_.superNameOffset)),
                decodeScalar<std::int32_t>(buffer, typeLayout_.isOopOffset) != 0,
                decodeScalar<std::int32_t>(buffer, typeLayout_.isIntegerOffset) != 0,
                decodeScalar<std::int32_t>(buffer, typeLayout_.isUnsignedOffset) != 0,
                decodeScalar<std::uint64_t>(buffer, typeLayout_.sizeOffset)
            });
        }
        return true;
    }

    bool vmStructs::loadIntConstants(const memory::processMemory &memory) {
        for (std::uint64_t index = 0;; ++index) {
            const auto buffer = memory.readBuffer(
                intConstantLayout_.array.address + index * intConstantLayout_.array.stride,
                static_cast<std::size_t>(intConstantLayout_.array.stride));
            const std::uint64_t nameAddress = decodeScalar<std::uint64_t>(buffer, intConstantLayout_.nameOffset);
            if (nameAddress == 0) {
                break;
            }

            constants_.add(vmIntConstant{
                readRemoteString(memory, nameAddress),
                decodeScalar<std::int32_t>(buffer, intConstantLayout_.valueOffset)
            });
        }
        return true;
    }

    bool vmStructs::loadLongConstants(const memory::processMemory &memory) {
        for (std::uint64_t index = 0;; ++index) {
            const auto buffer = memory.readBuffer(
                longConstantLayout_.array.address + index * longConstantLayout_.array.stride,
                static_cast<std::size_t>(longConstantLayout_.array.stride));
            const std::uint64_t nameAddress = decodeScalar<std::uint64_t>(buffer, longConstantLayout_.nameOffset);
            if (nameAddress == 0) {
                break;
            }

            constants_.add(vmLongConstant{
                readRemoteString(memory, nameAddress),
                decodeScalar<std::uint64_t>(buffer, longConstantLayout_.valueOffset)
            });
        }
        return true;
    }

    std::optional<std::uint64_t> vmStructs::readExportValue(
        const memory::remoteProcess &process,
        const memory::processMemory &memory,
        std::string_view exportName) const {
        try {
            const auto exportAddress = process.resolveExport(L"jvm.dll", exportName);
            return memory.read<std::uint64_t>(exportAddress);
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }

    std::string vmStructs::readRemoteString(const memory::processMemory &memory, std::uint64_t address) const {
        return address == 0 ? std::string{} : memory.readCString(address);
    }
}
