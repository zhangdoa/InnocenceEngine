#include "../Common/TestRunner.h"
#include "../../Engine/Common/Array.h"
#include "../../Engine/Common/DoubleBuffer.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace Inno;

static void TestDoubleBufferReadWriteFlipBasic()
{
	TestRunner::StartTest("DoubleBuffer: Write -> Flip -> Read sees written value");

	DoubleBuffer<int> db;
	db.Write([](int& back) { back = 42; });
	db.Flip();

	int observed = 0;
	db.Read([&](const int& front) { observed = front; });
	TestRunner::EndTest(observed == 42);
}

static void TestDoubleBufferConsumerSeesGenerations()
{
	TestRunner::StartTest("DoubleBuffer: SPSC 10^5 iterations, no torn reads");

	struct Payload { uint32_t a = 0; uint32_t b = 0; };
	DoubleBuffer<Payload> db;
	std::atomic<bool> stop{false};
	std::atomic<bool> failed{false};
	const uint32_t Iterations = 100000;

	// Prime both buffers so the first Read never sees default-uninitialised members.
	db.Write([](Payload& back) { back.a = 0; back.b = 0; });
	db.Flip();
	db.Write([](Payload& back) { back.a = 0; back.b = 0; });

	std::thread producer([&]() {
		for (uint32_t i = 1; i <= Iterations; ++i)
		{
			db.Write([&](Payload& back) { back.a = i; back.b = i; });
			db.Flip();
		}
		stop.store(true, std::memory_order_release);
	});

	while (!stop.load(std::memory_order_acquire))
	{
		db.Read([&](const Payload& front) {
			uint32_t snapA = front.a;
			uint32_t snapB = front.b;
			if (snapA != snapB) failed.store(true, std::memory_order_release);
		});
	}

	producer.join();

	db.Read([&](const Payload& front) {
		if (front.a != front.b) failed.store(true, std::memory_order_release);
		if (front.a == 0) failed.store(true, std::memory_order_release);
	});

	TestRunner::EndTest(!failed.load(std::memory_order_acquire));
}

static void TestDoubleBufferMultipleReaders()
{
	TestRunner::StartTest("DoubleBuffer: 4 concurrent readers, 1 producer + flip");

	DoubleBuffer<int> db;
	db.Write([](int& back) { back = 1; });
	db.Flip();

	std::atomic<bool> stop{false};
	std::atomic<bool> failed{false};
	const uint32_t Iterations = 50000;

	Inno::Array<std::thread> readers;
	for (int t = 0; t < 4; ++t)
	{
		readers.emplace_back([&]() {
			while (!stop.load(std::memory_order_acquire))
			{
				db.Read([&](const int& front) {
					if (front == 0) failed.store(true, std::memory_order_release);
				});
			}
		});
	}

	for (uint32_t i = 2; i <= Iterations; ++i)
	{
		db.Write([&](int& back) { back = static_cast<int>(i); });
		db.Flip();
	}
	stop.store(true, std::memory_order_release);

	for (auto& r : readers) r.join();
	TestRunner::EndTest(!failed.load(std::memory_order_acquire));
}

void RunDoubleBufferUnitTests()
{
	TestRunner::StartTestSuite("DoubleBuffer Unit Tests");

	TestDoubleBufferReadWriteFlipBasic();
	TestDoubleBufferConsumerSeesGenerations();
	TestDoubleBufferMultipleReaders();

	TestRunner::EndTestSuite();
}
