#pragma once
#include <iostream>
#include <string>
#include <vector>

struct TestRegistry {
    using TestFunc = bool (*) ();

    struct TestEntry {
        std::string Suite;
        std::string Name;
        TestFunc    Func;
    };

    static inline std::vector<TestEntry> Tests;

    static bool
    RunAll (const std::string &subsystemName) {
        size_t passed = 0;
        std::cout << "[======] Running tests for: " << subsystemName << " ("
                  << Tests.size () << " tests)\n";

        for (const auto &test : Tests) {
            std::cout << "[ RUN  ] " << test.Suite << '.' << test.Name << '\n';
            bool success = test.Func ();
            if (success) {
                std::cout << "[  OK  ]\n";
                ++passed;
            } else {
                std::cerr << "[ FAIL ]\n";
            }
        }
        std::cout << "[======] Passed: " << passed << '/' << Tests.size () << '\n';
        return passed == Tests.size ();
    }
};

#define test_func(suite, name)                                                           \
    bool Test##suite##name ();                                                           \
    static struct Register##suite##name {                                                \
        Register##suite##name () {                                                       \
            TestRegistry::Tests.emplace_back (#suite, #name, Test##suite##name);         \
        }                                                                                \
    } const reg##suite##name;                                                            \
    bool Test##suite##name ()

#define assert_eq(actual, expected)                                                      \
    if ((actual) != (expected)) {                                                        \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Expected: " << (expected) << ", got: " << (actual) << '\n';        \
        return false;                                                                    \
    }

#define assert_true(expr)                                                                \
    if (!(expr)) {                                                                       \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Condition is false: " << #expr << '\n';                            \
        return false;                                                                    \
    }

#define assert_snaphot_eq(actual, expected)                                              \
    if ((actual) != (expected)) {                                                        \
        std::cerr << "Snaphot mismatch at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Expected:\n"                                                       \
                  << (expected) << '\n'                                                  \
                  << "Actual:\n"                                                         \
                  << (actual) << '\n';                                                   \
        return false;                                                                    \
    }

#define assert_type_eq(actual, expected)                                                 \
    if ((actual) == nullptr || (expected) == nullptr || *(actual) != *(expected)) {      \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Types are not equal or one is nullptr\nExpected: " << (expected)   \
                  << ", got: " << (actual) << '\n';                                      \
        return false;                                                                    \
    }

#define assert_node_kind(node, expectedKind)                                             \
    if ((node) == nullptr || (node)->Kind () != (expectedKind)) {                        \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Expected node kind: " << (int) (expectedKind)                      \
                  << ", got: " << ((node) != nullptr ? (node)->Kind () : -1) << '\n';    \
    }

#define assert_diag(builder, code, message, severity)                                    \
    if ((builder).Code () != (code) || (builder).Message () != (message)                 \
        || (builder).GetSeverity () != (severity)) {                                     \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << '\n'       \
                  << "Expected diagnostic part: " << (int) (code) << '('                 \
                  << (int) (severity) << ") '" << (message)                              \
                  << "', got: " << (int) (builder).Code () << '('                        \
                  << (int) (builder).GetSeverity () << ") '" << (builder).Message ()     \
                  << "'\n";                                                              \
        return false;                                                                    \
    }
