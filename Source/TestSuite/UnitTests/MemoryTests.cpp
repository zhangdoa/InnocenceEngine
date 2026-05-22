#include "../Common/TestRunner.h"
#include "../../Engine/Common/Memory.h"
#include <cstring>
#include <cstdint>

using namespace Inno;

static void TestMemoryAllocateDeallocateRoundtrip()
{
	TestRunner::StartTest("Memory: Allocate / Deallocate round-trip");

	void* p = Memory::Allocate(1024);
	bool passed = (p != nullptr);
	if (p)
	{
		std::memset(p, 0xAB, 1024);
		auto* bytes = static_cast<uint8_t*>(p);
		passed = passed && (bytes[0] == 0xAB) && (bytes[1023] == 0xAB);
		Memory::Deallocate(p);
	}
	TestRunner::EndTest(passed);
}

static void TestMemoryReallocateGrow()
{
	TestRunner::StartTest("Memory: Reallocate grows + preserves prefix");

	auto* p = static_cast<uint8_t*>(Memory::Allocate(64));
	bool passed = (p != nullptr);
	if (p)
	{
		for (int i = 0; i < 64; ++i) p[i] = static_cast<uint8_t>(i);

		auto* p2 = static_cast<uint8_t*>(Memory::Reallocate(p, 256));
		passed = passed && (p2 != nullptr);
		if (p2)
		{
			for (int i = 0; i < 64; ++i)
			{
				if (p2[i] != static_cast<uint8_t>(i))
				{
					passed = false;
					break;
				}
			}
			Memory::Deallocate(p2);
		}
	}
	TestRunner::EndTest(passed);
}

static void TestMemoryReallocateShrink()
{
	TestRunner::StartTest("Memory: Reallocate shrinks + preserves prefix");

	auto* p = static_cast<uint8_t*>(Memory::Allocate(512));
	bool passed = (p != nullptr);
	if (p)
	{
		for (int i = 0; i < 512; ++i) p[i] = static_cast<uint8_t>(i & 0xFF);

		auto* p2 = static_cast<uint8_t*>(Memory::Reallocate(p, 128));
		passed = passed && (p2 != nullptr);
		if (p2)
		{
			for (int i = 0; i < 128; ++i)
			{
				if (p2[i] != static_cast<uint8_t>(i & 0xFF))
				{
					passed = false;
					break;
				}
			}
			Memory::Deallocate(p2);
		}
	}
	TestRunner::EndTest(passed);
}

static void TestMemoryReallocateFromNullActsAsAllocate()
{
	TestRunner::StartTest("Memory: Reallocate(nullptr, n) acts as Allocate(n)");

	void* p = Memory::Reallocate(nullptr, 256);
	bool passed = (p != nullptr);
	if (p) Memory::Deallocate(p);
	TestRunner::EndTest(passed);
}

static void TestMemoryDeallocateNullIsSafe()
{
	TestRunner::StartTest("Memory: Deallocate(nullptr) is a no-op");

	Memory::Deallocate(nullptr);
	TestRunner::EndTest(true);
}

void RunMemoryUnitTests()
{
	TestRunner::StartTestSuite("Memory Unit Tests");

	TestMemoryAllocateDeallocateRoundtrip();
	TestMemoryReallocateGrow();
	TestMemoryReallocateShrink();
	TestMemoryReallocateFromNullActsAsAllocate();
	TestMemoryDeallocateNullIsSafe();

	TestRunner::EndTestSuite();
}
