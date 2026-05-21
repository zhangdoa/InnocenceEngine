#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/ObjectPool.h"
#include "../../Engine/Common/Memory.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void TestObjectPoolMassiveAllocations()
{
	TestRunner::StartTest("ObjectPool Massive Allocations Stress");

	const size_t l_PoolSize = TestConfig::StressTestSize;
	auto l_ObjectPool = TObjectPool<TestStruct>::Create(l_PoolSize);
	bool l_TestPassed = true;

	std::vector<TestStruct*> l_Objects;
	l_Objects.reserve(l_PoolSize);

	for (size_t i = 0; i < l_PoolSize; i++)
	{
		auto l_Object = l_ObjectPool->Spawn();
		if (!l_Object)
		{
			l_TestPassed = false;
			break;
		}
		l_Objects.push_back(l_Object);
	}

	auto l_ExtraObject = l_ObjectPool->Spawn();
	if (l_ExtraObject != nullptr)
	{
		l_TestPassed = false;
	}

	for (auto l_Object : l_Objects)
	{
		l_ObjectPool->Destroy(l_Object);
	}

	for (size_t i = 0; i < l_PoolSize / 2; i++)
	{
		auto l_Object = l_ObjectPool->Spawn();
		if (!l_Object)
		{
			l_TestPassed = false;
			break;
		}
		l_ObjectPool->Destroy(l_Object);
	}

	TObjectPool<TestStruct>::Destruct(l_ObjectPool);
	TestRunner::EndTest(l_TestPassed);
}

void TestMemoryFragmentationStress()
{
	TestRunner::StartTest("Memory Fragmentation Stress");

	const size_t l_TestIterations = 1000;
	const size_t l_ObjectsPerIteration = 100;
	bool l_TestPassed = true;

	for (size_t i = 0; i < l_TestIterations; i++)
	{
		auto l_ObjectPool = TObjectPool<uint32_t>::Create(l_ObjectsPerIteration);
		std::vector<uint32_t*> l_Objects;

		for (size_t j = 0; j < l_ObjectsPerIteration; j++)
		{
			auto l_Object = l_ObjectPool->Spawn();
			if (!l_Object)
			{
				l_TestPassed = false;
				break;
			}
			l_Objects.push_back(l_Object);
		}

		// Randomized deallocation to create fragmentation before re-allocating.
		std::default_random_engine l_Generator;
		std::uniform_int_distribution<size_t> l_RandomIndex(0, l_Objects.size() - 1);
		
		for (size_t j = 0; j < l_ObjectsPerIteration / 2; j++)
		{
			auto l_Index = l_RandomIndex(l_Generator);
			if (l_Objects[l_Index])
			{
				l_ObjectPool->Destroy(l_Objects[l_Index]);
				l_Objects[l_Index] = nullptr;
			}
		}

		for (size_t j = 0; j < l_ObjectsPerIteration / 4; j++)
		{
			auto l_Object = l_ObjectPool->Spawn();
			if (!l_Object)
			{
				l_TestPassed = false;
				break;
			}
			l_ObjectPool->Destroy(l_Object);
		}

		TObjectPool<uint32_t>::Destruct(l_ObjectPool);

		if (!l_TestPassed)
		{
			break;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void RunMemoryStressTests()
{
	TestRunner::StartTestSuite("Memory Stress Tests");
	
	TestObjectPoolMassiveAllocations();
	TestMemoryFragmentationStress();
	
	TestRunner::EndTestSuite();
}
