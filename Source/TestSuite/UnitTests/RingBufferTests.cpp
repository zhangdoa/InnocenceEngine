#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/RingBuffer.h"
#include <atomic>
#include <thread>

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

void TestRingBufferThreadSafeProducerConsumer()
{
	TestRunner::StartTest("RingBuffer<T, true>: 1 producer + 1 consumer, no torn size()/operator[]");

	RingBuffer<int, true> rb;
	rb.reserve(64);
	std::atomic<bool> stop{false};
	std::atomic<bool> failed{false};
	const int Iterations = 50000;

	std::thread producer([&]() {
		for (int i = 1; i <= Iterations; ++i)
		{
			rb.emplace_back(i);
		}
		stop.store(true, std::memory_order_release);
	});

	while (!stop.load(std::memory_order_acquire))
	{
		size_t s = rb.size();
		if (s > rb.capacity()) failed.store(true, std::memory_order_release);
		for (size_t i = 0; i < s; ++i)
		{
			int v = rb[i];
			if (v < 0 || v > Iterations) failed.store(true, std::memory_order_release);
		}
	}

	producer.join();
	TestRunner::EndTest(!failed.load(std::memory_order_acquire));
}

void RunRingBufferUnitTests()
{
	TestRunner::StartTestSuite("RingBuffer Unit Tests");

	TestRingBufferBasicOperations();
	TestRingBufferWraparound();
	TestRingBufferThreadSafeProducerConsumer();

	TestRunner::EndTestSuite();
}
