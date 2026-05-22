#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Array.h"
#include "../../Engine/Common/Queue.h"
#include "../../Engine/Common/HashMap.h"
#include "../../Engine/Common/Allocator.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

#include <queue>
#include <unordered_map>
#include <vector>

using namespace Inno;

static void TestArrayPerformance()
{
	TestRunner::StartTest("Array vs std::vector — isolated: push_back, iterate, copy (N=LargeDataSize)");

	const size_t N = TestConfig::LargeDataSize;

	// Per-operation isolation. Each block reserves first so no growth happens
	// during push_back. Times in microseconds (ms × 1000).
	double innoPush = TestTimer::MeasureFunction([&]()
	{
		Array<float> a;
		a.reserve(N);
		for (size_t i = 0; i < N; ++i) a.emplace_back(static_cast<float>(i));
	});
	double stlPush = TestTimer::MeasureFunction([&]()
	{
		std::vector<float> v;
		v.reserve(N);
		for (size_t i = 0; i < N; ++i) v.emplace_back(static_cast<float>(i));
	});

	// Pre-fill once; benchmark just the iteration.
	Array<float> innoFilled;
	innoFilled.reserve(N);
	for (size_t i = 0; i < N; ++i) innoFilled.emplace_back(static_cast<float>(i));
	std::vector<float> stlFilled;
	stlFilled.reserve(N);
	for (size_t i = 0; i < N; ++i) stlFilled.emplace_back(static_cast<float>(i));

	double innoIter = TestTimer::MeasureFunction([&]()
	{
		float sum = 0;
		for (auto v : innoFilled) sum += v;
		volatile float sink = sum; (void)sink;
	});
	double stlIter = TestTimer::MeasureFunction([&]()
	{
		float sum = 0;
		for (auto v : stlFilled) sum += v;
		volatile float sink = sum; (void)sink;
	});

	double innoCopy = TestTimer::MeasureFunction([&]() { auto b = innoFilled; (void)b; });
	double stlCopy  = TestTimer::MeasureFunction([&]() { auto b = stlFilled;  (void)b; });

	// Raw alloc+memcpy+free comparison to isolate from container code.
	double rawMallocTime = TestTimer::MeasureFunction([&]()
	{
		void* p = std::malloc(N * sizeof(float));
		std::memcpy(p, innoFilled.data(), N * sizeof(float));
		std::free(p);
	});
	double rawNewTime = TestTimer::MeasureFunction([&]()
	{
		void* p = ::operator new(N * sizeof(float));
		std::memcpy(p, stlFilled.data(), N * sizeof(float));
		::operator delete(p);
	});

	Log(Success, "  push_back: Inno ", innoPush, "ms, STL ", stlPush, "ms (ratio ", innoPush / stlPush, ")");
	Log(Success, "  iterate  : Inno ", innoIter, "ms, STL ", stlIter, "ms (ratio ", innoIter / stlIter, ")");
	Log(Success, "  copy     : Inno ", innoCopy, "ms, STL ", stlCopy, "ms (ratio ", innoCopy / stlCopy, ")");
	Log(Success, "  raw alloc+memcpy+free: malloc ", rawMallocTime, "ms, operator new ", rawNewTime, "ms");
	TestRunner::EndTest(true);
}

static void TestQueuePerformance()
{
	TestRunner::StartTest("Queue vs std::queue — push N then pop N");

	const size_t N = TestConfig::MediumDataSize;

	auto innoTime = TestTimer::MeasureFunction([&]()
	{
		Queue<int> q;
		q.reserve(N);
		for (size_t i = 0; i < N; ++i) q.push(static_cast<int>(i));
		while (!q.empty()) q.pop();
	});

	auto stlTime = TestTimer::MeasureFunction([&]()
	{
		std::queue<int> q;
		for (size_t i = 0; i < N; ++i) q.push(static_cast<int>(i));
		while (!q.empty()) q.pop();
	});

	Log(Success, "Queue vs std::queue speed ratio: ", innoTime / stlTime, " (Inno ", innoTime, "ms, STL ", stlTime, "ms)");
	TestRunner::EndTest(true);
}

static void TestHashMapPerformance()
{
	TestRunner::StartTest("HashMap vs std::unordered_map — insert + lookup + erase");

	const size_t N = TestConfig::MediumDataSize;

	auto innoTime = TestTimer::MeasureFunction([&]()
	{
		HashMap<int, int> m;
		m.reserve(N);
		for (size_t i = 0; i < N; ++i) m.insert(static_cast<int>(i), static_cast<int>(i * 7));
		long long sum = 0;
		for (size_t i = 0; i < N; ++i)
		{
			auto it = m.find(static_cast<int>(i));
			if (it != m.end()) sum += it->second;
		}
		for (size_t i = 0; i < N; i += 2) m.erase(static_cast<int>(i));
		(void)sum;
	});

	auto stlTime = TestTimer::MeasureFunction([&]()
	{
		std::unordered_map<int, int> m;
		m.reserve(N);
		for (size_t i = 0; i < N; ++i) m.emplace(static_cast<int>(i), static_cast<int>(i * 7));
		long long sum = 0;
		for (size_t i = 0; i < N; ++i)
		{
			auto it = m.find(static_cast<int>(i));
			if (it != m.end()) sum += it->second;
		}
		for (size_t i = 0; i < N; i += 2) m.erase(static_cast<int>(i));
		(void)sum;
	});

	Log(Success, "HashMap vs std::unordered_map speed ratio: ", innoTime / stlTime, " (Inno ", innoTime, "ms, STL ", stlTime, "ms)");
	TestRunner::EndTest(true);
}

static void TestAllocatorPerformance()
{
	TestRunner::StartTest("std::vector<T, Inno::Allocator> vs std::vector<T> — push_back");

	const size_t N = TestConfig::MediumDataSize;

	auto innoTime = TestTimer::MeasureFunction([&]()
	{
		std::vector<int, Allocator<int>> v;
		v.reserve(N);
		for (size_t i = 0; i < N; ++i) v.emplace_back(static_cast<int>(i));
	});

	auto stlTime = TestTimer::MeasureFunction([&]()
	{
		std::vector<int> v;
		v.reserve(N);
		for (size_t i = 0; i < N; ++i) v.emplace_back(static_cast<int>(i));
	});

	Log(Success, "Inno::Allocator vs std::allocator speed ratio: ", innoTime / stlTime, " (Inno ", innoTime, "ms, STL ", stlTime, "ms)");
	TestRunner::EndTest(true);
}

void RunContainerPerformanceTests()
{
	TestRunner::StartTestSuite("Container Performance Tests");

	TestArrayPerformance();
	TestQueuePerformance();
	TestHashMapPerformance();
	TestAllocatorPerformance();

	TestRunner::EndTestSuite();
}
