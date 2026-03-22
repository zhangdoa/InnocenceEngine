#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/RingBuffer.h"

using namespace Inno;

void TestRingBufferBasicOperations()
{
	TestRunner::StartTest("RingBuffer Basic Operations");

	RingBuffer<float> l_RingBuffer;
	bool l_TestPassed = true;

	// Test reservation and basic operations
	l_RingBuffer.reserve(16);
	
	// Fill buffer
	for (size_t i = 0; i < 8; i++)
	{
		l_RingBuffer.emplace_back(static_cast<float>(i));
	}

	// Test size
	if (l_RingBuffer.size() != 8)
	{
		l_TestPassed = false;
	}

	// Test element access
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

	// Test with small buffer size
	l_RingBuffer.reserve(8);
	
	// Fill buffer beyond capacity to test wraparound
	for (size_t i = 0; i < 16; i++)
	{
		l_RingBuffer.emplace_back(static_cast<int32_t>(i));
	}

	// Buffer should contain the last 8 elements
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
