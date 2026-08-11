#include "testing.h"

#include <iostream>

namespace testing {
    namespace {
        int failureCount = 0;
    }

    std::vector<testCase> &registry() {
        static std::vector<testCase> cases;
        return cases;
    }

    registrar::registrar(std::string name, std::function<void()> body) {
        registry().push_back(testCase{std::move(name), std::move(body)});
    }

    void reportFailure(std::string_view expression, std::string_view file, int line, std::string_view detail) {
        ++failureCount;
        std::cout << "  FAIL " << file << ":" << line << " " << expression;
        if (!detail.empty()) {
            std::cout << " (" << detail << ")";
        }
        std::cout << '\n';
    }

    int run() {
        int failedCases = 0;
        for (const auto &entry: registry()) {
            const int before = failureCount;
            std::cout << entry.name << '\n';
            try {
                entry.body();
            } catch (const std::exception &exception) {
                reportFailure("unexpected exception", "-", 0, exception.what());
            }

            if (failureCount != before) {
                ++failedCases;
            }
        }

        std::cout << '\n'
                << registry().size() << " cases, " << failedCases << " failed, "
                << failureCount << " checks failed\n";
        return failureCount == 0 ? 0 : 1;
    }
}

int main() {
    return testing::run();
}
