#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/ObjectPool.h"
#include "../../Engine/Common/Memory.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void TestObjectPoolPerformance()
{
	TestRunner::StartTest("ObjectPool vs Malloc Performance");

	const size_t l_TestCount = TestConfig::LargeDataSize;
	std::vector<uint32_t*> l_ObjectsInPool(l_TestCount);
	std::vector<void*> l_ObjectsRaw(l_TestCount);

	auto l_ObjectPool = TObjectPool<uint32_t>::Create(l_TestCount);

	// Test ObjectPool allocation
	auto l_PoolAllocTime = TestTimer::MeasureFunction([&]()
	{
		for (size_t i = 0; i < l_TestCount; i++)
		{
			auto l_Ptr = l_ObjectPool->Spawn();
			l_ObjectsInPool[i] = l_Ptr;
		}
	});

	// Test malloc allocation
	auto l_MallocTime = TestTimer::MeasureFunction([&]()
	{
		for (size_t i = 0; i < l_TestCount; i++)
		{
			auto l_Ptr = malloc(sizeof(TestStruct));
			l_ObjectsRaw[i] = l_Ptr;
		}
	});

	auto l_AllocSpeedRatio = l_PoolAllocTime / l_MallocTime;
	Log(Success, "ObjectPool allocation vs malloc speed ratio: ", l_AllocSpeedRatio);

	// Test ObjectPool deallocation
	auto l_PoolDeallocTime = TestTimer::MeasureFunction([&]()
	{
		for (size_t i = 0; i < l_TestCount; i++)
		{
			l_ObjectPool->Destroy(l_ObjectsInPool[i]);
		}
	});

	// Test free deallocation
	auto l_FreeTime = TestTimer::MeasureFunction([&]()
	{
		for (size_t i = 0; i < l_TestCount; i++)
		{
			free(l_ObjectsRaw[i]);
		}
	});

	auto l_DeallocSpeedRatio = l_PoolDeallocTime / l_FreeTime;
	Log(Success, "ObjectPool deallocation vs free speed ratio: ", l_DeallocSpeedRatio);

	TObjectPool<uint32_t>::Clear(l_ObjectPool);
	TObjectPool<uint32_t>::Destruct(l_ObjectPool);

	TestRunner::EndTest(true);
}

void RunMemoryPerformanceTests()
{
	TestRunner::StartTestSuite("Memory Performance Tests");
	
	TestObjectPoolPerformance();
	
	TestRunner::EndTestSuite();
}
