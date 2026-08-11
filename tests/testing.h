#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace testing {
    struct testCase {
        std::string name;
        std::function<void()> body;
    };

    std::vector<testCase> &registry();

    struct registrar {
        registrar(std::string name, std::function<void()> body);
    };

    void reportFailure(std::string_view expression, std::string_view file, int line, std::string_view detail);

    template<typename left, typename right>
    void checkEqual(const left &actual, const right &expected, std::string_view expression,
                    std::string_view file, int line) {
        if (!(actual == expected)) {
            std::ostringstream detail;
            detail << "actual [" << actual << "] expected [" << expected << "]";
            reportFailure(expression, file, line, detail.str());
        }
    }

    int run();
}

#define SPLINTER_JOIN_INNER(left, right) left##right
#define SPLINTER_JOIN(left, right) SPLINTER_JOIN_INNER(left, right)

#define TEST(name)                                                                       \
    static void SPLINTER_JOIN(splinterTest, __LINE__)();                                 \
    static const ::testing::registrar SPLINTER_JOIN(splinterRegistrar, __LINE__){        \
        name, SPLINTER_JOIN(splinterTest, __LINE__)};                                    \
    static void SPLINTER_JOIN(splinterTest, __LINE__)()

#define CHECK(expression)                                                                \
    do {                                                                                 \
        if (!(expression)) {                                                             \
            ::testing::reportFailure(#expression, __FILE__, __LINE__, "");                \
        }                                                                                \
    } while (false)

#define CHECK_EQUAL(actual, expected)                                                    \
    ::testing::checkEqual((actual), (expected), #actual " == " #expected, __FILE__, __LINE__)

#define CHECK_THROWS(expression)                                                         \
    do {                                                                                 \
        bool splinterThrew = false;                                                      \
        try {                                                                            \
            (void) (expression);                                                         \
        } catch (...) {                                                                  \
            splinterThrew = true;                                                        \
        }                                                                                \
        if (!splinterThrew) {                                                            \
            ::testing::reportFailure(#expression " throws", __FILE__, __LINE__, "");      \
        }                                                                                \
    } while (false)
