#include "../Common/TestRunner.h"
#include "../../Engine/Common/Array.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/ObjectPool.h"
#include "../../Engine/Common/Memory.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void TestObjectPoolBasicOperations()
{
	TestRunner::StartTest("ObjectPool Basic Operations");

	auto l_ObjectPool = TObjectPool<uint32_t>::Create(100);
	bool l_TestPassed = true;

	Inno::Array<uint32_t*> l_Objects;
	for (size_t i = 0; i < 50; i++)
	{
		auto l_Object = l_ObjectPool->Spawn();
		if (!l_Object)
		{
			l_TestPassed = false;
			break;
		}
		*l_Object = static_cast<uint32_t>(i);
		l_Objects.push_back(l_Object);
	}

	for (auto l_Object : l_Objects)
	{
		l_ObjectPool->Destroy(l_Object);
	}

	for (size_t i = 0; i < 25; i++)
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
	TestRunner::EndTest(l_TestPassed);
}

void TestObjectPoolExhaustion()
{
	TestRunner::StartTest("ObjectPool Exhaustion");

	auto l_ObjectPool = TObjectPool<uint32_t>::Create(10);
	bool l_TestPassed = true;

	Inno::Array<uint32_t*> l_Objects;

	for (size_t i = 0; i < 10; i++)
	{
		auto l_Object = l_ObjectPool->Spawn();
		if (l_Object)
		{
			l_Objects.push_back(l_Object);
		}
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

	TObjectPool<uint32_t>::Destruct(l_ObjectPool);
	TestRunner::EndTest(l_TestPassed);
}

void TestObjectPoolNullHandling()
{
	TestRunner::StartTest("ObjectPool Null Handling");

	auto l_ObjectPool = TObjectPool<uint32_t>::Create(10);
	bool l_TestPassed = true;

	// Test destroying null pointer (should not crash)
	l_ObjectPool->Destroy(nullptr);

	TObjectPool<uint32_t>::Destruct(l_ObjectPool);
	TestRunner::EndTest(l_TestPassed);
}

void TestObjectPoolSlotReuseZeroInitialised()
{
	TestRunner::StartTest("ObjectPool: spawn after destroy yields zero-initialised slot");

	auto l_ObjectPool = TObjectPool<uint32_t>::Create(4);
	bool l_TestPassed = true;

	auto l_First = l_ObjectPool->Spawn();
	if (!l_First)
	{
		l_TestPassed = false;
	}
	else
	{
		*l_First = 0xDEADBEEF;
		l_ObjectPool->Destroy(l_First);

		auto l_Second = l_ObjectPool->Spawn();
		if (!l_Second)
		{
			l_TestPassed = false;
		}
		else
		{
			// Destroy memsets the T region to 0, then Spawn placement-new T() zero-inits.
			l_TestPassed = (*l_Second == 0);
			l_ObjectPool->Destroy(l_Second);
		}
	}

	TObjectPool<uint32_t>::Destruct(l_ObjectPool);
	TestRunner::EndTest(l_TestPassed);
}

void RunObjectPoolUnitTests()
{
	TestRunner::StartTestSuite("ObjectPool Unit Tests");

	TestObjectPoolBasicOperations();
	TestObjectPoolExhaustion();
	TestObjectPoolNullHandling();
	TestObjectPoolSlotReuseZeroInitialised();

	TestRunner::EndTestSuite();
}
