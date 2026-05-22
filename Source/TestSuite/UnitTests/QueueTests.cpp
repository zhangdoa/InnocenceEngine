#include "../Common/TestRunner.h"
#include "../../Engine/Common/Queue.h"
#include <string>
#include <utility>

using namespace Inno;

static void TestQueueFIFO()
{
	TestRunner::StartTest("Queue: push/pop preserves FIFO order");

	Queue<int> q;
	for (int i = 0; i < 20; ++i) q.push(i);
	bool passed = (q.size() == 20);
	for (int i = 0; i < 20 && passed; ++i)
	{
		if (q.front() != i) passed = false;
		q.pop();
	}
	passed = passed && q.empty();
	TestRunner::EndTest(passed);
}

static void TestQueueGrowsAcrossWrap()
{
	TestRunner::StartTest("Queue: grow handles wrap (push N, pop K, push N more, drain in order)");

	Queue<int> q;
	q.reserve(4);
	for (int i = 0; i < 4; ++i) q.push(i);
	q.pop(); q.pop();          // drain 0,1 → head=2, size=2
	for (int i = 4; i < 12; ++i) q.push(i);  // forces grow past cap=4 with wrapped contents

	bool passed = (q.size() == 10);
	int expected = 2;
	while (!q.empty() && passed)
	{
		if (q.front() != expected++) passed = false;
		q.pop();
	}
	passed = passed && (expected == 12);
	TestRunner::EndTest(passed);
}

static void TestQueueNonTrivialT()
{
	TestRunner::StartTest("Queue<std::string>: growth preserves non-trivial elements");

	Queue<std::string> q;
	for (int i = 0; i < 100; ++i)
		q.emplace(std::string("msg-") + std::to_string(i));

	bool passed = (q.size() == 100);
	for (int i = 0; i < 100 && passed; ++i)
	{
		if (q.front() != ("msg-" + std::to_string(i))) passed = false;
		q.pop();
	}
	TestRunner::EndTest(passed);
}

static void TestQueueCopyMove()
{
	TestRunner::StartTest("Queue: copy + move ctors + assignment");

	Queue<int> a;
	for (int i = 0; i < 10; ++i) a.push(i);

	Queue<int> b = a;
	bool passed = (b.size() == 10);
	for (int i = 0; i < 10 && passed; ++i)
	{
		if (b.front() != i) passed = false;
		b.pop();
	}

	Queue<int> c(std::move(a));
	passed = passed && (c.size() == 10) && (a.size() == 0);
	for (int i = 0; i < 10 && passed; ++i)
	{
		if (c.front() != i) passed = false;
		c.pop();
	}

	TestRunner::EndTest(passed);
}

static void TestQueueClear()
{
	TestRunner::StartTest("Queue: clear empties without freeing");

	Queue<int> q;
	for (int i = 0; i < 10; ++i) q.push(i);
	const size_t cap = q.capacity();
	q.clear();
	bool passed = q.empty() && (q.size() == 0) && (q.capacity() == cap);
	q.push(42);
	passed = passed && (q.size() == 1) && (q.front() == 42);
	TestRunner::EndTest(passed);
}

void RunQueueUnitTests()
{
	TestRunner::StartTestSuite("Queue Unit Tests");

	TestQueueFIFO();
	TestQueueGrowsAcrossWrap();
	TestQueueNonTrivialT();
	TestQueueCopyMove();
	TestQueueClear();

	TestRunner::EndTestSuite();
}
