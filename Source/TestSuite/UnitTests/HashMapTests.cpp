#include "../Common/TestRunner.h"
#include "../../Engine/Common/HashMap.h"

#include <string>

using namespace Inno;

static void TestHashMapInsertFind()
{
	TestRunner::StartTest("HashMap: insert + find");

	HashMap<int, int> m;
	for (int i = 0; i < 100; ++i) m.insert(i, i * 7);
	bool passed = (m.size() == 100);
	for (int i = 0; i < 100 && passed; ++i)
	{
		auto it = m.find(i);
		if (it == m.end() || it->second != i * 7) passed = false;
	}
	if (m.find(999) != m.end()) passed = false;
	TestRunner::EndTest(passed);
}

static void TestHashMapInsertOrAssign()
{
	TestRunner::StartTest("HashMap: insert_or_assign returns false on update");

	HashMap<int, int> m;
	bool inserted = m.insert_or_assign(1, 100);
	bool updated  = m.insert_or_assign(1, 200);
	bool passed = inserted && !updated && (m.size() == 1) && (m.find(1)->second == 200);
	TestRunner::EndTest(passed);
}

static void TestHashMapErase()
{
	TestRunner::StartTest("HashMap: erase + reinsert through tombstone");

	HashMap<int, int> m;
	for (int i = 0; i < 20; ++i) m.insert(i, i);
	bool passed = (m.size() == 20);
	for (int i = 0; i < 20; i += 2)
		if (!m.erase(i)) passed = false;
	passed = passed && (m.size() == 10);
	for (int i = 1; i < 20 && passed; i += 2)
		if (!m.contains(i)) passed = false;
	for (int i = 0; i < 20 && passed; i += 2)
		if (m.contains(i)) passed = false;
	for (int i = 0; i < 20; i += 2)
		if (!m.insert(i, i * 10)) passed = false;
	for (int i = 0; i < 20 && passed; i += 2)
		if (m.find(i)->second != i * 10) passed = false;
	TestRunner::EndTest(passed);
}

static void TestHashMapStringKey()
{
	TestRunner::StartTest("HashMap<std::string, int>: non-trivial Key works through rehash");

	HashMap<std::string, int> m;
	for (int i = 0; i < 500; ++i)
		m.insert(std::string("key-") + std::to_string(i), i);
	bool passed = (m.size() == 500);
	for (int i = 0; i < 500 && passed; ++i)
	{
		auto it = m.find(std::string("key-") + std::to_string(i));
		if (it == m.end() || it->second != i) passed = false;
	}
	TestRunner::EndTest(passed);
}

static void TestHashMapOperatorBracket()
{
	TestRunner::StartTest("HashMap: operator[] inserts default on miss");

	HashMap<int, int> m;
	m[5] = 100;
	int defaulted = m[6];
	bool passed = (defaulted == 0) && (m.size() == 2) && (m.find(5)->second == 100);
	TestRunner::EndTest(passed);
}

static void TestHashMapCopyMove()
{
	TestRunner::StartTest("HashMap: copy + move ctors + assignment");

	HashMap<int, int> a;
	for (int i = 0; i < 10; ++i) a.insert(i, i * 2);

	HashMap<int, int> b = a;
	bool passed = (b.size() == 10);
	for (int i = 0; i < 10 && passed; ++i)
		if (!b.contains(i) || b.find(i)->second != i * 2) passed = false;

	HashMap<int, int> c(std::move(a));
	passed = passed && (c.size() == 10) && (a.size() == 0);
	for (int i = 0; i < 10 && passed; ++i)
		if (!c.contains(i) || c.find(i)->second != i * 2) passed = false;

	TestRunner::EndTest(passed);
}

static void TestHashMapIteration()
{
	TestRunner::StartTest("HashMap: range-based-for visits every occupied slot exactly once");

	HashMap<int, int> m;
	for (int i = 0; i < 50; ++i) m.insert(i, i * 3);
	for (int i = 0; i < 50; i += 5) m.erase(i);  // sprinkle tombstones

	int visited = 0;
	bool passed = true;
	for (auto& kv : m)
	{
		if (kv.second != kv.first * 3) passed = false;
		if ((kv.first % 5) == 0) passed = false;  // erased keys must not appear
		++visited;
	}
	passed = passed && (visited == static_cast<int>(m.size()));
	TestRunner::EndTest(passed);
}

void RunHashMapUnitTests()
{
	TestRunner::StartTestSuite("HashMap Unit Tests");

	TestHashMapInsertFind();
	TestHashMapInsertOrAssign();
	TestHashMapErase();
	TestHashMapStringKey();
	TestHashMapOperatorBracket();
	TestHashMapCopyMove();
	TestHashMapIteration();

	TestRunner::EndTestSuite();
}
