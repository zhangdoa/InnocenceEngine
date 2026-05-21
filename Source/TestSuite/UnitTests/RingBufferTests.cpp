#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/RingBuffer.h"

using namespace Inno;

void TestRingBufferBasicOperations()
{
	TestRunner::StartTest("RingBuffer Basic Operations");

	RingBuffer<float> l_RingBuffer;
	bool l_TestPassed = true;

	l_RingBuffer.reserve(16);

	for (size_t i = 0; i < 8; i++)
	{
		l_RingBuffer.emplace_back(static_cast<float>(i));
	}

	if (l_RingBuffer.size() != 8)
	{
		l_TestPassed = false;
	}

	for (size_t i = 0; i < 8; i++)
	{
		if (l_RingBuffer[i] != static_cast<float>(i))
		{
			l_TestPassed = false;
			break;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestRingBufferWraparound()
{
	TestRunner::StartTest("RingBuffer Wraparound");

	RingBuffer<int32_t> l_RingBuffer;
	bool l_TestPassed = true;

	l_RingBuffer.reserve(8);

	for (size_t i = 0; i < 16; i++)
	{
		l_RingBuffer.emplace_back(static_cast<int32_t>(i));
	}

	// Wraparound: only the last 8 of 16 inserted elements survive.
	if (l_RingBuffer.size() != 8)
	{
		l_TestPassed = false;
	}

	TestRunner::EndTest(l_TestPassed);
}

void RunRingBufferUnitTests()
{
	TestRunner::StartTestSuite("RingBuffer Unit Tests");
	
	TestRingBufferBasicOperations();
	TestRingBufferWraparound();
	
	TestRunner::EndTestSuite();
}
