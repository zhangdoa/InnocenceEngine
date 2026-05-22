#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/RingBuffer.h"

using namespace Inno;

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

	TestRingBufferStress();

	TestRunner::EndTestSuite();
}
