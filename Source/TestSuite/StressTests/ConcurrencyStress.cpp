#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Atomic.h"
#include "../../Engine/Common/RingBuffer.h"

using namespace Inno;

void TestConcurrentAtomicOperations()
{
	TestRunner::StartTest("Concurrent Atomic Operations Stress");

	Atomic<uint32_t> l_AtomicBuffer;
	std::atomic<uint32_t> l_FinishedTaskCount{0};
	bool l_TestPassed = true;

	// Initialize atomic buffer
	{
		auto l_Writer = AtomicWriter(l_AtomicBuffer);
		*l_Writer.Get() = 0;
	}

	const size_t l_ThreadCount = TestConfig::ConcurrencyTestThreads;
	const size_t l_OperationsPerThread = 100;

	std::vector<std::thread> l_Threads;
	l_Threads.reserve(l_ThreadCount);

	for (size_t i = 0; i < l_ThreadCount; i++)
	{
		l_Threads.emplace_back([&]()
		{
			std::default_random_engine l_Generator;
			std::uniform_int_distribution<uint32_t> l_RandomDelta(1, 10);

			for (size_t j = 0; j < l_OperationsPerThread; j++)
			{
				auto l_ExecutionTime = l_RandomDelta(l_Generator);

				// Read operation
				{
					auto l_Reader = AtomicReader(l_AtomicBuffer);
					volatile auto l_Value = *l_Reader.Get();
					(void)l_Value; // Suppress unused variable warning
				}

				// Write operation
				{
					auto l_Writer = AtomicWriter(l_AtomicBuffer);
					*l_Writer.Get() += l_ExecutionTime;
				}

				// Another read to stress test
				{
					auto l_Reader = AtomicReader(l_AtomicBuffer);
					volatile auto l_Value = *l_Reader.Get();
					(void)l_Value; // Suppress unused variable warning
				}
			}

			l_FinishedTaskCount++;
		});
	}

	// Wait for all threads to complete
	for (auto& l_Thread : l_Threads)
	{
		l_Thread.join();
	}

	// Verify all tasks completed
	if (l_FinishedTaskCount != l_ThreadCount)
	{
		l_TestPassed = false;
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestRingBufferStress()
{
	TestRunner::StartTest("RingBuffer Stress Test");

	const size_t l_TestIterations = 128;
	bool l_TestPassed = true;

	std::default_random_engine l_Generator;
	std::uniform_int_distribution<uint32_t> l_RandomSize(8, 16);

	for (size_t i = 0; i < l_TestIterations; i++)
	{
		RingBuffer<float> l_RingBuffer;
		auto l_BufferSize = static_cast<size_t>(std::pow(2, l_RandomSize(l_Generator)));
		l_RingBuffer.reserve(l_BufferSize);
		
		auto l_TestTime = l_BufferSize * 4;
		for (size_t j = 0; j < l_TestTime; j++)
		{
			l_RingBuffer.emplace_back(static_cast<float>(j));
		}

		// Verify buffer integrity
		if (l_RingBuffer.size() != l_BufferSize)
		{
			l_TestPassed = false;
			break;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void RunConcurrencyStressTests()
{
	TestRunner::StartTestSuite("Concurrency Stress Tests");
	
	TestConcurrentAtomicOperations();
	TestRingBufferStress();
	
	TestRunner::EndTestSuite();
}
