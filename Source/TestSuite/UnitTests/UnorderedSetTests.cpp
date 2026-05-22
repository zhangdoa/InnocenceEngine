#include "../Common/TestRunner.h"
#include "../../Engine/Common/UnorderedSet.h"

#include <string>

using namespace Inno;

static void TestUnorderedSetInsertContains()
{
	TestRunner::StartTest("UnorderedSet: insert + contains + find");

	UnorderedSet<int> s;
	for (int i = 0; i < 100; ++i) s.insert(i);
	bool passed = (s.size() == 100);
	for (int i = 0; i < 100 && passed; ++i)
	{
		if (!s.contains(i)) passed = false;
		auto it = s.find(i);
		if (it == s.end() || *it != i) passed = false;
	}
	if (s.contains(999)) passed = false;
	if (s.find(999) != s.end()) passed = false;
	TestRunner::EndTest(passed);
}

static void TestUnorderedSetDedup()
{
	TestRunner::StartTest("UnorderedSet: re-inserting key returns false, size stays");

	UnorderedSet<int> s;
	bool inserted = s.insert(42);
	bool reinsert = s.insert(42);
	bool passed = inserted && !reinsert && (s.size() == 1);
	TestRunner::EndTest(passed);
}

static void TestUnorderedSetEraseReinsert()
{
	TestRunner::StartTest("UnorderedSet: erase + reinsert through tombstone");

	UnorderedSet<int> s;
	for (int i = 0; i < 20; ++i) s.insert(i);
	bool passed = (s.size() == 20);
	for (int i = 0; i < 20; i += 2)
		if (!s.erase(i)) passed = false;
	passed = passed && (s.size() == 10);
	for (int i = 1; i < 20 && passed; i += 2)
		if (!s.contains(i)) passed = false;
	for (int i = 0; i < 20 && passed; i += 2)
		if (s.contains(i)) passed = false;
	for (int i = 0; i < 20; i += 2)
		if (!s.insert(i)) passed = false;
	for (int i = 0; i < 20 && passed; ++i)
		if (!s.contains(i)) passed = false;
	TestRunner::EndTest(passed);
}

static void TestUnorderedSetIteration()
{
	TestRunner::StartTest("UnorderedSet: range-for visits every element exactly once");

	UnorderedSet<int> s;
	for (int i = 0; i < 50; ++i) s.insert(i);
	for (int i = 0; i < 50; i += 5) s.erase(i);  // sprinkle tombstones

	int visited = 0;
	bool seen[50] = {};
	bool passed = true;
	for (const auto& v : s)
	{
		if (v < 0 || v >= 50) { passed = false; break; }
		if ((v % 5) == 0) { passed = false; break; }  // erased keys must not appear
		if (seen[v]) { passed = false; break; }
		seen[v] = true;
		++visited;
	}
	passed = passed && (visited == static_cast<int>(s.size()));
	TestRunner::EndTest(passed);
}

static void TestUnorderedSetRangeAndInitListCtors()
{
	TestRunner::StartTest("UnorderedSet: range + initializer_list ctors");

	int src[] = {1, 2, 3, 4, 5, 3, 2};  // dups in input must be deduped
	UnorderedSet<int> a(src, src + 7);
	bool passed = (a.size() == 5);
	for (int v : {1, 2, 3, 4, 5}) if (!a.contains(v)) passed = false;

	UnorderedSet<int> b = {10, 20, 30, 20};
	passed = passed && (b.size() == 3) && b.contains(10) && b.contains(20) && b.contains(30);

	TestRunner::EndTest(passed);
}

static void TestUnorderedSetCopyMove()
{
	TestRunner::StartTest("UnorderedSet: copy + move ctors + assignment");

	UnorderedSet<int> a;
	for (int i = 0; i < 10; ++i) a.insert(i * 2);

	UnorderedSet<int> b = a;
	bool passed = (b.size() == 10);
	for (int i = 0; i < 10 && passed; ++i)
		if (!b.contains(i * 2)) passed = false;

	UnorderedSet<int> c(std::move(a));
	passed = passed && (c.size() == 10) && (a.size() == 0);
	for (int i = 0; i < 10 && passed; ++i)
		if (!c.contains(i * 2)) passed = false;

	TestRunner::EndTest(passed);
}

struct ConstantHasher
{
	std::size_t operator()(int) const noexcept { return 0; }  // collide every key
};

static void TestUnorderedSetCustomHash()
{
	TestRunner::StartTest("UnorderedSet: custom Hash + KeyEqual templated correctly");

	UnorderedSet<int, ConstantHasher> s;
	for (int i = 0; i < 50; ++i) s.insert(i);
	bool passed = (s.size() == 50);
	for (int i = 0; i < 50 && passed; ++i)
		if (!s.contains(i)) passed = false;
	for (int i = 0; i < 50; i += 2)
		if (!s.erase(i)) passed = false;
	for (int i = 0; i < 50 && passed; ++i)
	{
		bool wantHave = (i % 2) == 1;
		if (s.contains(i) != wantHave) passed = false;
	}
	TestRunner::EndTest(passed);
}

static void TestUnorderedSetStringKey()
{
	TestRunner::StartTest("UnorderedSet<std::string>: non-trivial T works through rehash");

	UnorderedSet<std::string> s;
	for (int i = 0; i < 500; ++i)
		s.insert(std::string("key-") + std::to_string(i));
	bool passed = (s.size() == 500);
	for (int i = 0; i < 500 && passed; ++i)
		if (!s.contains(std::string("key-") + std::to_string(i))) passed = false;
	TestRunner::EndTest(passed);
}

void RunUnorderedSetUnitTests()
{
	TestRunner::StartTestSuite("UnorderedSet Unit Tests");

	TestUnorderedSetInsertContains();
	TestUnorderedSetDedup();
	TestUnorderedSetEraseReinsert();
	TestUnorderedSetIteration();
	TestUnorderedSetRangeAndInitListCtors();
	TestUnorderedSetCopyMove();
	TestUnorderedSetCustomHash();
	TestUnorderedSetStringKey();

	TestRunner::EndTestSuite();
}
