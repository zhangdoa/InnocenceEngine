#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Atomic.h"

using namespace Inno;

void TestAtomicBasicOperations()
{
	TestRunner::StartTest("Atomic Basic Operations");

	Atomic<uint32_t> l_AtomicValue;
	bool l_TestPassed = true;

	// Test basic read/write
	{
		auto l_Writer = AtomicWriter(l_AtomicValue);
		*l_Writer.Get() = 42;
	}

	{
		auto l_Reader = AtomicReader(l_AtomicValue);
		if (*l_Reader.Get() != 42)
		{
			l_TestPassed = false;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestAtomicConcurrentAccess()
{
	TestRunner::StartTest("Atomic Concurrent Access");

	Atomic<uint32_t> l_AtomicBuffer;
	bool l_TestPassed = true;

	// Initialize value
	{
		auto l_Writer = AtomicWriter(l_AtomicBuffer);
		*l_Writer.Get() = 0;
	}

	// Test concurrent read/write operations
	std::vector<std::thread> l_Threads;
	const size_t l_ThreadCount = 4;
	const size_t l_OperationsPerThread = 100;

	for (size_t i = 0; i < l_ThreadCount; i++)
	{
		l_Threads.emplace_back([&l_AtomicBuffer, l_OperationsPerThread]()
		{
			for (size_t j = 0; j < l_OperationsPerThread; j++)
			{
				// Read operation
				{
					auto l_Reader = AtomicReader(l_AtomicBuffer);
					volatile auto l_Value = *l_Reader.Get();
					(void)l_Value; // Suppress unused variable warning
				}

				// Write operation
				{
					auto l_Writer = AtomicWriter(l_AtomicBuffer);
					(*l_Writer.Get())++;
				}
			}
		});
	}

	// Wait for all threads to complete
	for (auto& l_Thread : l_Threads)
	{
		l_Thread.join();
	}

	// Verify final value
	{
		auto l_Reader = AtomicReader(l_AtomicBuffer);
		auto l_FinalValue = *l_Reader.Get();
		if (l_FinalValue != l_ThreadCount * l_OperationsPerThread)
		{
			l_TestPassed = false;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void RunAtomicUnitTests()
{
	TestRunner::StartTestSuite("Atomic Unit Tests");
	
	TestAtomicBasicOperations();
	TestAtomicConcurrentAccess();
	
	TestRunner::EndTestSuite();
}
