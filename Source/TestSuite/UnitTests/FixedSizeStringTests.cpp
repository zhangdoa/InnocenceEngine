// FixedSizeString unit and regression tests.
// Covers construction, assignment, comparison, find/size, overflow clamping,
// hash specialisation, and the known off-by-one behaviour (non-empty strings
// have their last character replaced by '\0' — this is intentional and relied
// upon by the JSON serialisation layer: instance names use a trailing '/' as
// the sacrificial character so the stored name has no path separator).
//
// The empty-string guard (strlen==0 → write m_content[0]='\\0') is the bug
// that was fixed in this codebase; those paths are covered as regression tests.

#include "../Common/TestRunner.h"
#include "../../Engine/Common/FixedSizeString.h"
#include <unordered_map>
#include <string>

using namespace Inno;

// ---------------------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------------------
static void TestFSSDefaultConstruction()
{
    TestRunner::StartTest("FixedSizeString: default construction");

    FixedSizeString<64> s;
    // Default-constructed — m_content is uninitialized, but c_str() pointer is valid.
    // We only verify the object is constructible and usable.
    bool passed = (s.c_str() != nullptr);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Construction from non-empty const char*
// ---------------------------------------------------------------------------
static void TestFSSConstructFromCStr()
{
    TestRunner::StartTest("FixedSizeString: construct from non-empty const char*");

    // Known off-by-one: the last character of the input is replaced with '\0'.
    // "hello" (5 chars) → c_str() returns "hell" (4 printable chars + NUL).
    // This is documented intentional behaviour; the trailing-slash convention
    // (e.g. "MyComp/") exploits it so the stored name is "MyComp".
    FixedSizeString<64> s("hello/");
    bool passed = (std::string(s.c_str()) == "hello");
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Regression: construct from empty string must not underflow the buffer
// ---------------------------------------------------------------------------
static void TestFSSConstructFromEmptyString()
{
    TestRunner::StartTest("FixedSizeString: construct from empty string (regression — no buffer underflow)");

    // Before the fix, strlen("")==0 caused m_content[0-1]=m_content[-1]='\\0'
    // → heap corruption.  After the fix the empty-string guard writes m_content[0].
    FixedSizeString<64> s("");
    bool passed = (s.c_str() != nullptr) && (s.size() == 0);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Assignment from non-empty const char*
// ---------------------------------------------------------------------------
static void TestFSSAssignFromCStr()
{
    TestRunner::StartTest("FixedSizeString: operator= from non-empty const char*");

    FixedSizeString<64> s;
    s = "world/";
    bool passed = (std::string(s.c_str()) == "world");
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Regression: operator= from empty string must not underflow
// ---------------------------------------------------------------------------
static void TestFSSAssignFromEmptyString()
{
    TestRunner::StartTest("FixedSizeString: operator= from empty string (regression — no buffer underflow)");

    FixedSizeString<64> s("nonempty/");
    s = "";
    bool passed = (s.c_str() != nullptr) && (s.size() == 0);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Copy construction
// ---------------------------------------------------------------------------
static void TestFSSCopyConstruction()
{
    TestRunner::StartTest("FixedSizeString: copy construction");

    FixedSizeString<64> a("copy/");
    FixedSizeString<64> b(a);
    bool passed = (b == a) && (std::string(b.c_str()) == std::string(a.c_str()));
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Copy assignment
// ---------------------------------------------------------------------------
static void TestFSSCopyAssignment()
{
    TestRunner::StartTest("FixedSizeString: copy assignment");

    FixedSizeString<64> a("assign/");
    FixedSizeString<64> b;
    b = a;
    bool passed = (b == a);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Equality and inequality operators
// ---------------------------------------------------------------------------
static void TestFSSEqualityOperators()
{
    TestRunner::StartTest("FixedSizeString: equality and inequality operators");

    FixedSizeString<64> a("alpha/");
    FixedSizeString<64> b("alpha/");
    FixedSizeString<64> c("beta/");

    bool passed =
        (a == b) &&
        !(a != b) &&
        (a != c) &&
        !(a == c) &&
        (a == "alpha") &&  // off-by-one: stored "alpha", compare "alpha" ✓
        (a != "beta");

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// size() reflects stored content length (after off-by-one truncation)
// ---------------------------------------------------------------------------
static void TestFSSSize()
{
    TestRunner::StartTest("FixedSizeString: size() after off-by-one truncation");

    // "hello/" → stored "hello" → size() == 5
    FixedSizeString<64> s("hello/");
    bool passed = (s.size() == 5);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// find() returns non-null for present substring, null for absent
// ---------------------------------------------------------------------------
static void TestFSSFind()
{
    TestRunner::StartTest("FixedSizeString: find() present and absent substrings");

    FixedSizeString<64> s("Component/");  // stored: "Component"
    bool passed =
        (s.find("Comp") != nullptr) &&
        (s.find("xyz")  == nullptr) &&
        (s.find("")     != nullptr);  // strstr(x,"") is always non-null

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Overflow clamping: input longer than S is silently truncated
// ---------------------------------------------------------------------------
static void TestFSSOverflowClamping()
{
    TestRunner::StartTest("FixedSizeString: overflow clamping");

    // S=8, input has 20 chars — must not overflow the 8-byte buffer.
    // strlen clamped to 8; m_content[7]='\\0' (off-by-one within S).
    FixedSizeString<8> s("12345678901234567890");
    // size() <= 7 (at most 7 chars before the NUL inserted at [7])
    bool passed = (s.size() <= 7) && (s.c_str() != nullptr);
    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// Hash specialisation: usable as unordered_map key
// ---------------------------------------------------------------------------
static void TestFSSHashUsability()
{
    TestRunner::StartTest("FixedSizeString: hash specialisation — usable as unordered_map key");

    std::unordered_map<FixedSizeString<64>, int> map;
    FixedSizeString<64> k1("key1/");
    FixedSizeString<64> k2("key2/");
    map[k1] = 42;
    map[k2] = 99;

    bool passed = (map.count(k1) == 1) && (map[k1] == 42) &&
                  (map.count(k2) == 1) && (map[k2] == 99);

    TestRunner::EndTest(passed);
}

// ---------------------------------------------------------------------------
// int32/int64 specialisations (ToString helpers)
// ---------------------------------------------------------------------------
static void TestFSSIntegerSpecialisations()
{
    TestRunner::StartTest("FixedSizeString: int32/int64 specialisations via ToString");

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
    TestFSSConstructFromEmptyString();
    TestFSSAssignFromCStr();
    TestFSSAssignFromEmptyString();
    TestFSSCopyConstruction();
    TestFSSCopyAssignment();
    TestFSSEqualityOperators();
    TestFSSSize();
    TestFSSFind();
    TestFSSOverflowClamping();
    TestFSSHashUsability();
    TestFSSIntegerSpecialisations();

    TestRunner::EndTestSuite();
}
