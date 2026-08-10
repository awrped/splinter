#include "classLoaderData.h"

#include <unordered_set>

namespace splinter::engine::hotspot {
    namespace {
        // the graph is walked without suspending the target, a link that is torn or
        // freed mid walk can point back into the chain and loop forever
        constexpr std::size_t maxChainLength = 1u << 20;
    }

    classLoaderDataView::classLoaderDataView(const memory::processMemory &memory, const vmStructs &vm,
                                             std::uint64_t address) noexcept
        : memory_(&memory), vm_(&vm), address_(address) {
    }

    std::uint64_t classLoaderDataView::address() const noexcept {
        return address_;
    }

    std::optional<std::uint64_t> classLoaderDataView::nextAddress() const {
        const vmStructEntry *field = vm_->findField("ClassLoaderData", "_next");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::optional<std::uint64_t> classLoaderDataView::klassesAddress() const {
        const vmStructEntry *field = vm_->findField("ClassLoaderData", "_klasses");
        return field != nullptr
                   ? std::optional<std::uint64_t>(memory_->read<std::uint64_t>(address_ + field->offset))
                   : std::nullopt;
    }

    std::vector<std::uint64_t> classLoaderDataView::klassAddresses(std::size_t limit) const {
        std::vector<std::uint64_t> result;
        const auto firstKlass = klassesAddress();
        const vmStructEntry *nextLinkField = vm_->findField("Klass", "_next_link");
        if (!firstKlass || nextLinkField == nullptr) {
            return result;
        }

        std::unordered_set<std::uint64_t> visited;
        std::uint64_t current = *firstKlass;
        while (current != 0 && result.size() < maxChainLength) {
            if (!visited.insert(current).second) {
                break;
            }

            result.push_back(current);
            if (limit != 0 && result.size() >= limit) {
                break;
            }
            current = memory_->read<std::uint64_t>(current + nextLinkField->offset);
        }
        return result;
    }

    std::optional<std::uint64_t> classLoaderDataView::headAddress(const memory::processMemory &memory,
                                                                  const vmStructs &vm) {
        const vmStructEntry *field = vm.findField("ClassLoaderDataGraph", "_head");
        if (field == nullptr || !field->isStatic) {
            return std::nullopt;
        }
        return memory.read<std::uint64_t>(field->address);
    }

    std::vector<std::uint64_t> classLoaderDataView::enumerate(const memory::processMemory &memory,
                                                              const vmStructs &vm,
                                                              std::size_t classLoaderLimit,
                                                              std::size_t klassLimit) {
        std::vector<std::uint64_t> result;
        const vmStructEntry *nextField = vm.findField("ClassLoaderData", "_next");
        const auto head = headAddress(memory, vm);
        if (nextField == nullptr || !head) {
            return result;
        }

        std::unordered_set<std::uint64_t> visited;
        std::uint64_t current = *head;
        std::size_t classLoaderCount = 0;
        while (current != 0 && classLoaderCount < maxChainLength) {
            if (!visited.insert(current).second) {
                break;
            }

            classLoaderDataView view(memory, vm, current);
            auto klasses = view.klassAddresses(klassLimit == 0 ? 0 : klassLimit - result.size());
            result.insert(result.end(), klasses.begin(), klasses.end());
            if (klassLimit != 0 && result.size() >= klassLimit) {
                break;
            }

            ++classLoaderCount;
            if (classLoaderLimit != 0 && classLoaderCount >= classLoaderLimit) {
                break;
            }

            current = memory.read<std::uint64_t>(current + nextField->offset);
        }
        return result;
    }
}