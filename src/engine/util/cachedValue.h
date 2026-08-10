#pragma once

#include <optional>
#include <utility>

namespace splinter::engine::util {
    // memoizes one lazily computed value so a view can stay const while avoiding
    // repeated reads of the same remote field
    template<typename value>
    class cachedValue {
    public:
        template<typename compute>
        const value &get(compute &&produce) const {
            if (!stored_.has_value()) {
                stored_ = std::forward<compute>(produce)();
            }
            return *stored_;
        }

        void reset() noexcept {
            stored_.reset();
        }

    private:
        mutable std::optional<value> stored_;
    };
}
