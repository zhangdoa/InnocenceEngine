// FixedSizeString unit and regression tests.
//
// Rewritten for the 2026-04-17 cleanup that removed the "sacrificial
// trailing character" off-by-one. The class now:
//   - Always null-terminates (including default construction).
//   - Stores exactly what was passed (up to capacity N-1 chars + NUL).
//   - Is nullptr-safe on const char* assignment.
//   - Rejects implicit int -> string via deleted integer constructors;
//     use ToString(int32_t) / ToString(int64_t) instead.

#include "../Common/TestRunner.h"
#include "../../Engine/Common/FixedSizeString.h"
#include <unordered_map>
#include <string>
#include <cstring>

using namespace Inno;

// ---------------------------------------------------------------------------
// Default construction yields an empty, null-terminated string.
// ---------------------------------------------------------------------------
static void TestFSSDefaultConstruction()
{
    TestRunner::StartTest("FixedSizeString: default-construct is empty and null-terminated");

    FixedSizeString<64> s;
    bool passed = (s.c_str() != nullptr) && (s.empty()) && (s.size() == 0) && (s.c_str()[0] == '\0');
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Construction from non-empty const char* preserves the full string.
// ---------------------------------------------------------------------------
static void TestFSSConstructFromCStr()
{
    TestRunner::StartTest("FixedSizeString: construct from non-empty const char* preserves all chars");

    FixedSizeString<64> s("hello");
    bool passed = (std::string(s.c_str()) == "hello") && (s.size() == 5);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Trailing slash is now preserved (was previously eaten as sacrificial char).
// ---------------------------------------------------------------------------
static void TestFSSTrailingSlashPreserved()
{
    TestRunner::StartTest("FixedSizeString: trailing slash is preserved (no sacrificial-char truncation)");

    FixedSizeString<64> s("Component/");
    bool passed = (std::string(s.c_str()) == "Component/") && (s.size() == 10);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Construction from empty string yields empty, null-terminated string.
// ---------------------------------------------------------------------------
static void TestFSSConstructFromEmptyString()
{
    TestRunner::StartTest("FixedSizeString: construct from empty string is safe");

    FixedSizeString<64> s("");
    bool passed = (s.size() == 0) && s.empty() && (s.c_str()[0] == '\0');
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Null pointer input is handled gracefully.
// ---------------------------------------------------------------------------
static void TestFSSConstructFromNullPtr()
{
    TestRunner::StartTest("FixedSizeString: construct from nullptr yields empty string");

    const char* np = nullptr;
    FixedSizeString<64> s(np);
    bool passed = s.empty() && (s.size() == 0);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Assignment from non-empty const char* preserves the full string.
// ---------------------------------------------------------------------------
static void TestFSSAssignFromCStr()
{
    TestRunner::StartTest("FixedSizeString: operator= from non-empty const char* preserves all chars");

    FixedSizeString<64> s;
    s = "world!";
    bool passed = (std::string(s.c_str()) == "world!") && (s.size() == 6);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Assignment from empty string resets cleanly.
// ---------------------------------------------------------------------------
static void TestFSSAssignFromEmptyString()
{
    TestRunner::StartTest("FixedSizeString: operator= from empty string resets to empty");

    FixedSizeString<64> s("nonempty");
    s = "";
    bool passed = s.empty() && (s.size() == 0);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Copy construction and copy assignment.
// ---------------------------------------------------------------------------
static void TestFSSCopy()
{
    TestRunner::StartTest("FixedSizeString: copy construction and copy assignment preserve content");

    FixedSizeString<64> a("copyable/value");
    FixedSizeString<64> b(a);
    FixedSizeString<64> c;
    c = a;

    bool passed =
        (b == a) &&
        (std::string(b.c_str()) == std::string(a.c_str())) &&
        (c == a) &&
        (std::string(c.c_str()) == "copyable/value");
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Equality and inequality with const char* and with other FixedSizeStrings.
// ---------------------------------------------------------------------------
static void TestFSSEqualityOperators()
{
    TestRunner::StartTest("FixedSizeString: equality and inequality operators");

    FixedSizeString<64> a("alpha");
    FixedSizeString<64> b("alpha");
    FixedSizeString<64> c("beta");

    bool passed =
        (a == b) && !(a != b) &&
        (a != c) && !(a == c) &&
        (a == "alpha") && (a != "beta") &&
        !(a == static_cast<const char*>(nullptr));  // nullptr rhs is never equal
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Boundary: input exactly fills the buffer (capacity = S-1 chars).
// ---------------------------------------------------------------------------
static void TestFSSCapacityBoundary()
{
    TestRunner::StartTest("FixedSizeString<8>: input of exactly capacity (7 chars) stored intact");

    FixedSizeString<8> s("1234567");  // 7 chars = capacity
    bool passed = (s.size() == 7) && (std::string(s.c_str()) == "1234567");
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Boundary: input longer than capacity is truncated, not overflowed.
// ---------------------------------------------------------------------------
static void TestFSSOverflowClamping()
{
    TestRunner::StartTest("FixedSizeString<8>: input longer than capacity is truncated to capacity");

    // S=8 → capacity=7. Input is 20 chars. Must truncate to 7 chars + NUL.
    FixedSizeString<8> s("12345678901234567890");
    bool passed = (s.size() == 7) && (std::string(s.c_str()) == "1234567");
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// find() returns non-null for present substring, null for absent.
// ---------------------------------------------------------------------------
static void TestFSSFind()
{
    TestRunner::StartTest("FixedSizeString: find() present and absent substrings");

    FixedSizeString<64> s("Component/child");
    bool passed =
        (s.find("Comp")  != nullptr) &&
        (s.find("child") != nullptr) &&
        (s.find("/")     != nullptr) &&
        (s.find("xyz")   == nullptr);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Usable as unordered_map key via the hash specialisation.
// ---------------------------------------------------------------------------
static void TestFSSHashUsability()
{
    TestRunner::StartTest("FixedSizeString: usable as unordered_map key");

    std::unordered_map<FixedSizeString<64>, int> map;
    FixedSizeString<64> k1("key1");
    FixedSizeString<64> k2("key2");
    map[k1] = 42;
    map[k2] = 99;

    bool passed = (map.count(k1) == 1) && (map[k1] == 42) &&
                  (map.count(k2) == 1) && (map[k2] == 99);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Integer -> string via explicit ToString helpers.
// ---------------------------------------------------------------------------
static void TestFSSIntegerToString()
{
    TestRunner::StartTest("FixedSizeString: integer -> string via ToString helpers");

    auto s32 = ToString(static_cast<int32_t>(12345));
    auto s64 = ToString(static_cast<int64_t>(-9876543210LL));

    bool passed =
        (std::string(s32.c_str()) == "12345") &&
        (s64.find("9876543210") != nullptr);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
void RunFixedSizeStringUnitTests()
{
    TestRunner::StartTestSuite("FixedSizeString Unit Tests");

    TestFSSDefaultConstruction();
    TestFSSConstructFromCStr();
    TestFSSTrailingSlashPreserved();
    TestFSSConstructFromEmptyString();
    TestFSSConstructFromNullPtr();
    TestFSSAssignFromCStr();
    TestFSSAssignFromEmptyString();
    TestFSSCopy();
    TestFSSEqualityOperators();
    TestFSSCapacityBoundary();
    TestFSSOverflowClamping();
    TestFSSFind();
    TestFSSHashUsability();
    TestFSSIntegerToString();

    TestRunner::EndTestSuite();
}
