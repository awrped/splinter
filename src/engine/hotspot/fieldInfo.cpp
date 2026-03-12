#include "fieldInfo.h"

#include "symbolTable.h"

#include <algorithm>
#include <stdexcept>

namespace splinter::engine::hotspot {
    bool fieldFlags::isInitialized() const noexcept {
        return (raw & (1u << 0)) != 0;
    }

    bool fieldFlags::isInjected() const noexcept {
        return (raw & (1u << 1)) != 0;
    }

    bool fieldFlags::isGeneric() const noexcept {
        return (raw & (1u << 2)) != 0;
    }

    bool fieldFlags::isStable() const noexcept {
        return (raw & (1u << 3)) != 0;
    }

    bool fieldFlags::isContended() const noexcept {
        return (raw & (1u << 4)) != 0;
    }

    fieldInfoView::fieldInfoView(const memory::processMemory &memory, const vmStructs &vm,
                                 std::uint64_t streamAddress) noexcept
        : memory_(&memory), vm_(&vm), address_(streamAddress) {
    }

    std::uint64_t fieldInfoView::address() const noexcept {
        return address_;
    }

    std::optional<std::uint32_t> fieldInfoView::javaFieldCount() const {
        if (address_ == 0) {
            return std::nullopt;
        }

        const auto stream = readStream();
        unsigned5Reader reader(stream);
        return reader.hasNext() ? std::optional<std::uint32_t>(reader.nextUint()) : std::nullopt;
    }

    std::optional<std::uint32_t> fieldInfoView::injectedFieldCount() const {
        if (address_ == 0) {
            return std::nullopt;
        }

        const auto stream = readStream();
        unsigned5Reader reader(stream);
        if (!reader.hasNext()) {
            return std::nullopt;
        }
        const auto ignoredJavaFieldCount = reader.nextUint();
        (void) ignoredJavaFieldCount;
        return reader.hasNext() ? std::optional<std::uint32_t>(reader.nextUint()) : std::nullopt;
    }

    std::optional<std::uint32_t> fieldInfoView::totalFieldCount() const {
        const auto javaFields = javaFieldCount();
        const auto injectedFields = injectedFieldCount();
        if (!javaFields || !injectedFields) {
            return std::nullopt;
        }
        return *javaFields + *injectedFields;
    }

    std::vector<decodedFieldInfo> fieldInfoView::decodeAll(const constantPoolView &constantPool,
                                                           const symbolTable &symbols,
                                                           std::size_t limit) const {
        std::vector<decodedFieldInfo> fields;
        if (address_ == 0) {
            return fields;
        }

        const auto stream = readStream();
        unsigned5Reader reader(stream);
        if (!reader.hasNext()) {
            return fields;
        }

        const std::uint32_t javaFields = reader.nextUint();
        const std::uint32_t injectedFields = reader.hasNext() ? reader.nextUint() : 0;
        const std::uint32_t totalFields = javaFields + injectedFields;
        const std::size_t count = limit == 0
                                      ? static_cast<std::size_t>(totalFields)
                                      : std::min(limit, static_cast<std::size_t>(totalFields));

        fields.reserve(count);
        for (std::uint32_t index = 0; index < count && reader.hasNext(); ++index) {
            fields.push_back(decodeOne(reader, index, constantPool, symbols));
        }

        return fields;
    }

    fieldInfoView::unsigned5Reader::unsigned5Reader(const std::vector<std::byte> &buffer) noexcept
        : buffer_(&buffer) {
    }

    bool fieldInfoView::unsigned5Reader::hasNext() const noexcept {
        return buffer_ != nullptr && position_ < buffer_->size() &&
               std::to_integer<std::uint8_t>((*buffer_)[position_]) != 0;
    }

    std::size_t fieldInfoView::unsigned5Reader::position() const noexcept {
        return position_;
    }

    std::uint32_t fieldInfoView::unsigned5Reader::nextUint() {
        if (!hasNext()) {
            throw std::runtime_error("UNSIGNED5 reader reached the end of the fieldinfo stream");
        }

        const std::size_t start = position_;
        std::uint32_t first = std::to_integer<std::uint8_t>((*buffer_)[position_]);
        // HotSpot's UNSIGNED5 encoding reserves byte value 0 as the stream terminator.
        if (first < 1) {
            throw std::runtime_error("Encountered an excluded byte in the fieldinfo stream");
        }

        std::uint32_t sum = first - 1;
        if (sum < 191) {
            ++position_;
            return sum;
        }

        std::uint32_t shift = 6;
        for (std::uint32_t index = 1; index < 5; ++index) {
            if (start + index >= buffer_->size()) {
                throw std::runtime_error("Truncated UNSIGNED5 value in the fieldinfo stream");
            }

            const std::uint32_t value = std::to_integer<std::uint8_t>((*buffer_)[start + index]);
            if (value < 1) {
                throw std::runtime_error("Encountered an excluded byte in the fieldinfo stream");
            }

            sum += (value - 1) << shift;
            if (value < 192 || index == 4) {
                position_ = start + index + 1;
                return sum;
            }

            shift += 6;
        }

        throw std::runtime_error("Invalid UNSIGNED5 value in the fieldinfo stream");
    }

    std::vector<std::byte> fieldInfoView::readStream() const {
        const vmStructEntry *lengthField = vm_->findField("Array<int>", "_length");
        const vmStructEntry *dataField = vm_->findField("Array<u1>", "_data");
        if (lengthField == nullptr || dataField == nullptr) {
            throw std::runtime_error("Missing Array<u1> VMStruct metadata for fieldinfo decoding");
        }

        const std::int32_t length = memory_->read<std::int32_t>(address_ + lengthField->offset);
        if (length <= 0) {
            return {};
        }

        return memory_->readBuffer(address_ + dataField->offset, static_cast<std::size_t>(length));
    }

    decodedFieldInfo fieldInfoView::decodeOne(unsigned5Reader &reader,
                                              std::uint32_t index,
                                              const constantPoolView &constantPool,
                                              const symbolTable &symbols) const {
        decodedFieldInfo field{};
        field.index = index;
        field.nameIndex = static_cast<std::uint16_t>(reader.nextUint());
        field.signatureIndex = static_cast<std::uint16_t>(reader.nextUint());
        field.offset = reader.nextUint();
        field.accessFlags = reader.nextUint();
        field.flags.raw = reader.nextUint();

        if (field.flags.isInitialized()) {
            field.initializerIndex = static_cast<std::uint16_t>(reader.nextUint());
        }
        if (field.flags.isGeneric()) {
            field.genericSignatureIndex = static_cast<std::uint16_t>(reader.nextUint());
        }
        if (field.flags.isContended()) {
            field.contentionGroup = static_cast<std::uint16_t>(reader.nextUint());
        }

        if (field.nameIndex != 0) {
            field.name = constantPool.utf8At(field.nameIndex, symbols);
        }
        if (field.signatureIndex != 0) {
            field.signature = constantPool.utf8At(field.signatureIndex, symbols);
        }
        if (field.genericSignatureIndex != 0) {
            field.genericSignature = constantPool.utf8At(field.genericSignatureIndex, symbols);
        }

        return field;
    }
}
