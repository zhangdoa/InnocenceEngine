#include "../Common/TestRunner.h"
#include "../../Engine/Common/Deque.h"

#include <string>
#include <utility>

using namespace Inno;

static void TestDequeEmplaceAndIndex()
{
	TestRunner::StartTest("Deque: emplace_back + operator[] across chunks");

	Deque<int> d;
	const int N = 1000;  // > kChunkSize=64
	for (int i = 0; i < N; ++i) d.emplace_back(i * 3);
	bool passed = (d.size() == static_cast<size_t>(N));
	for (int i = 0; i < N && passed; ++i)
		if (d[i] != i * 3) passed = false;
	TestRunner::EndTest(passed);
}

static void TestDequePointerStability()
{
	TestRunner::StartTest("Deque: &deque[0] stable across emplace_back of 10x kChunkSize");

	Deque<int> d;
	d.emplace_back(42);
	int* captured = &d[0];
	for (int i = 0; i < 640; ++i) d.emplace_back(i);  // 10 chunks of 64
	bool passed = (captured == &d[0]) && (*captured == 42);
	TestRunner::EndTest(passed);
}

struct CountingDtor
{
	static int s_LiveCount;
	CountingDtor() { ++s_LiveCount; }
	CountingDtor(const CountingDtor&) { ++s_LiveCount; }
	CountingDtor(CountingDtor&&) noexcept { ++s_LiveCount; }
	~CountingDtor() { --s_LiveCount; }
	CountingDtor& operator=(const CountingDtor&) = default;
	CountingDtor& operator=(CountingDtor&&) noexcept = default;
};
int CountingDtor::s_LiveCount = 0;

static void TestDequeClearDestroys()
{
	TestRunner::StartTest("Deque: clear() destroys T (verified by live-count)");

	CountingDtor::s_LiveCount = 0;
	{
		Deque<CountingDtor> d;
		for (int i = 0; i < 200; ++i) d.emplace_back();
		bool atFull = (CountingDtor::s_LiveCount == 200);
		d.clear();
		bool zeroAfter = (CountingDtor::s_LiveCount == 0);
		TestRunner::EndTest(atFull && zeroAfter && d.size() == 0);
	}
}

static void TestDequeSizeAcrossChunkBoundary()
{
	TestRunner::StartTest("Deque: size() crosses chunk boundary correctly");

	Deque<int> d;
	bool passed = d.empty() && d.size() == 0;
	for (int i = 0; i < 64; ++i) d.emplace_back(i);
	passed = passed && (d.size() == 64);
	d.emplace_back(100);
	passed = passed && (d.size() == 65) && (d[64] == 100);
	TestRunner::EndTest(passed);
}

static void TestDequeMove()
{
	TestRunner::StartTest("Deque: move ctor + move-assign transfer storage");

	Deque<int> a;
	for (int i = 0; i < 100; ++i) a.emplace_back(i * 2);
	int* captured = &a[50];

	Deque<int> b(std::move(a));
	bool passed = (b.size() == 100) && (a.size() == 0) && (&b[50] == captured);
	for (int i = 0; i < 100 && passed; ++i)
		if (b[i] != i * 2) passed = false;

	Deque<int> c;
	c.emplace_back(999);
	c = std::move(b);
	passed = passed && (c.size() == 100) && (b.size() == 0);
	for (int i = 0; i < 100 && passed; ++i)
		if (c[i] != i * 2) passed = false;

	TestRunner::EndTest(passed);
}

static void TestDequeCopyIndependent()
{
	TestRunner::StartTest("Deque: copy ctor produces independent storage");

	Deque<int> a;
	for (int i = 0; i < 80; ++i) a.emplace_back(i);
	Deque<int> b(a);
	bool passed = (b.size() == a.size());
	for (int i = 0; i < 80 && passed; ++i)
		if (b[i] != a[i]) passed = false;
	a[10] = 9999;
	passed = passed && (b[10] == 10) && (a[10] == 9999);
	TestRunner::EndTest(passed);
}

void RunDequeUnitTests()
{
	TestRunner::StartTestSuite("Deque Unit Tests");

	TestDequeEmplaceAndIndex();
	TestDequePointerStability();
	TestDequeClearDestroys();
	TestDequeSizeAcrossChunkBoundary();
	TestDequeMove();
	TestDequeCopyIndependent();

	TestRunner::EndTestSuite();
}
