#include "moduleMap.h"

#include <cwctype>

namespace splinter::engine::memory {
    void moduleMap::clear() noexcept {
        modules_.clear();
    }

    void moduleMap::add(moduleInfo module) {
        modules_.push_back(std::move(module));
    }

    const std::vector<moduleInfo> &moduleMap::modules() const noexcept {
        return modules_;
    }

    const moduleInfo *moduleMap::findByName(std::wstring_view moduleName) const noexcept {
        for (const auto &module: modules_) {
            if (module.name.size() != moduleName.size()) {
                continue;
            }

            bool matches = true;
            for (std::size_t index = 0; index < module.name.size(); ++index) {
                if (std::towlower(module.name[index]) != std::towlower(moduleName[index])) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                return &module;
            }
        }

        return nullptr;
    }
}
