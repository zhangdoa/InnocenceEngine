#include "../Common/TestRunner.h"
#include "../../Engine/Common/Allocator.h"
#include <cstdint>
#include <new>

using namespace Inno;

static void TestAllocatorAllocateDeallocate()
{
	TestRunner::StartTest("Allocator: allocate + deallocate roundtrip");

	Allocator<uint32_t> alloc;
	uint32_t* p = alloc.allocate(64);
	bool passed = (p != nullptr);
	if (p)
	{
		for (size_t i = 0; i < 64; ++i) p[i] = static_cast<uint32_t>(i * 7);
		for (size_t i = 0; i < 64; ++i) passed = passed && (p[i] == static_cast<uint32_t>(i * 7));
		alloc.deallocate(p, 64);
	}
	TestRunner::EndTest(passed);
}

static void TestAllocatorRelatedConstruction()
{
	TestRunner::StartTest("Allocator: copy-construct from related-T allocator");

	Allocator<uint32_t> a;
	Allocator<uint64_t> b(a);
	(void)b;
	TestRunner::EndTest(true);
}

static void TestAllocatorOverflowThrows()
{
	TestRunner::StartTest("Allocator: allocate(huge N) throws bad_alloc");

	Allocator<uint64_t> alloc;
	bool threw = false;
	try
	{
		size_t huge = static_cast<size_t>(-1) / sizeof(uint64_t) + 1;
		auto* p = alloc.allocate(huge);
		(void)p;
	}
	catch (const std::bad_alloc&)
	{
		threw = true;
	}
	TestRunner::EndTest(threw);
}

static void TestAllocatorEquality()
{
	TestRunner::StartTest("Allocator: equality is always true (stateless)");

	Allocator<int> a;
	Allocator<int> b;
	Allocator<float> c;
	bool passed = (a == b) && !(a != b) && (a == c) && !(a != c);
	TestRunner::EndTest(passed);
}

void RunAllocatorUnitTests()
{
	TestRunner::StartTestSuite("Allocator Unit Tests");

	TestAllocatorAllocateDeallocate();
	TestAllocatorRelatedConstruction();
	TestAllocatorOverflowThrows();
	TestAllocatorEquality();

	TestRunner::EndTestSuite();
}
