#include "../Common/TestRunner.h"
#include "../../Engine/Common/Array.h"
#include "../../Engine/Common/Queue.h"
#include "../../Engine/Common/HashMap.h"

#include <cstdint>

using namespace Inno;

static void TestArray1MStress()
{
	TestRunner::StartTest("Array: 10^6 push_back + iterate + clear");

	Array<uint32_t> a;
	for (uint32_t i = 0; i < 1000000; ++i) a.push_back(i);

	bool passed = (a.size() == 1000000);
	uint64_t sum = 0;
	for (auto v : a) sum += v;
	// Sum of 0..999999 = 999999 * 1000000 / 2 = 499999500000.
	passed = passed && (sum == 499999500000ULL);

	a.clear();
	passed = passed && a.empty() && (a.size() == 0);

	TestRunner::EndTest(passed);
}

static void TestQueue1MStress()
{
	TestRunner::StartTest("Queue: 10^6 push then drain in FIFO order");

	Queue<uint32_t> q;
	for (uint32_t i = 0; i < 1000000; ++i) q.push(i);
	bool passed = (q.size() == 1000000);

	uint32_t expected = 0;
	while (!q.empty())
	{
		if (q.front() != expected) { passed = false; break; }
		q.pop();
		++expected;
	}
	passed = passed && (expected == 1000000);

	TestRunner::EndTest(passed);
}

static void TestHashMap100KStress()
{
	// 10^6 with std::pair<int,int> = 8 bytes/entry × ~4 (capacity vs size) = ~32MB.
	// HashMap<string, int> at 10^6 keys would be much larger. Use 10^5 — substantial
	// but bounded.
	TestRunner::StartTest("HashMap: 10^5 insert + lookup + 50% erase");

	HashMap<int, int> m;
	m.reserve(100000);
	for (int i = 0; i < 100000; ++i) m.insert(i, i * 3);
	bool passed = (m.size() == 100000);

	uint64_t sum = 0;
	for (int i = 0; i < 100000 && passed; ++i)
	{
		auto it = m.find(i);
		if (it == m.end()) passed = false;
		else sum += static_cast<uint64_t>(it->second);
	}
	// Sum of i*3 for i in 0..99999 = 3 * 99999 * 100000 / 2.
	passed = passed && (sum == 3ULL * 99999ULL * 100000ULL / 2ULL);

	for (int i = 0; i < 100000; i += 2)
		if (!m.erase(i)) { passed = false; break; }
	passed = passed && (m.size() == 50000);

	TestRunner::EndTest(passed);
}

void RunContainerStressTests()
{
	TestRunner::StartTestSuite("Container Stress Tests");

	TestArray1MStress();
	TestQueue1MStress();
	TestHashMap100KStress();

	TestRunner::EndTestSuite();
}
