#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Array.h"

#include <string>
#include <utility>

using namespace Inno;

void TestArrayBasicOperations()
{
	TestRunner::StartTest("Array Basic Operations");

	Array<int32_t> l_Array;
	bool l_TestPassed = true;

	l_Array.reserve(100);
	for (size_t i = 0; i < 50; i++)
	{
		l_Array.emplace_back(static_cast<int32_t>(i));
	}

	if (l_Array.size() != 50)
	{
		l_TestPassed = false;
	}

	for (size_t i = 0; i < 50; i++)
	{
		if (l_Array[i] != static_cast<int32_t>(i))
		{
			l_TestPassed = false;
			break;
		}
	}

	auto l_ArrayCopy = l_Array;
	if (l_ArrayCopy.size() != l_Array.size())
	{
		l_TestPassed = false;
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestArrayIterators()
{
	TestRunner::StartTest("Array Iterators");

	Array<float> l_Array;
	bool l_TestPassed = true;

	l_Array.reserve(25);
	for (size_t i = 0; i < 20; i++)
	{
		l_Array.emplace_back(static_cast<float>(i));
	}

	size_t l_Index = 0;
	for (auto& l_Value : l_Array)
	{
		if (l_Value != static_cast<float>(l_Index))
		{
			l_TestPassed = false;
			break;
		}
		l_Index++;
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestArrayFixedSizeDesign()
{
	TestRunner::StartTest("Array Fixed Size Design");

	bool l_TestPassed = true;

	Array<int> l_EmptyArray;
	if (l_EmptyArray.size() != 0 || l_EmptyArray.capacity() != 0 || l_EmptyArray.is_initialized())
	{
		l_TestPassed = false;
	}

	size_t l_IterCount = 0;
	for (auto& value : l_EmptyArray)
	{
		l_IterCount++;
	}
	if (l_IterCount != 0)
	{
		l_TestPassed = false;
	}

	Array<int> l_Array;
	l_Array.reserve(50);
	if (!l_Array.is_initialized() || l_Array.capacity() != 50 || l_Array.size() != 0)
	{
		l_TestPassed = false;
	}

	for (size_t i = 0; i < 30; i++)
	{
		l_Array.emplace_back(static_cast<int>(i * 2));
	}

	if (l_Array.size() != 30 || l_Array.capacity() != 50)
	{
		l_TestPassed = false;
	}

	for (size_t i = 0; i < 30; i++)
	{
		if (l_Array[i] != static_cast<int>(i * 2))
		{
			l_TestPassed = false;
			break;
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

static void TestArrayGrowFromEmpty()
{
	TestRunner::StartTest("Array: grows from empty (no reserve)");

	Array<int> a;
	for (int i = 0; i < 100; ++i) a.push_back(i);
	bool passed = (a.size() == 100) && (a.capacity() >= 100);
	for (int i = 0; i < 100 && passed; ++i)
		if (a[i] != i) passed = false;
	TestRunner::EndTest(passed);
}

static void TestArrayGrowPastReserve()
{
	TestRunner::StartTest("Array: grows past initial reserve()");

	Array<int> a;
	a.reserve(16);
	const size_t initialCap = a.capacity();
	for (int i = 0; i < 200; ++i) a.emplace_back(i * 3);
	bool passed = (a.size() == 200) && (a.capacity() > initialCap);
	for (int i = 0; i < 200 && passed; ++i)
		if (a[i] != i * 3) passed = false;
	TestRunner::EndTest(passed);
}

static void TestArrayNonTrivialT()
{
	TestRunner::StartTest("Array<std::string>: growth preserves non-trivially-copyable elements");

	Array<std::string> a;
	for (int i = 0; i < 50; ++i)
		a.emplace_back(std::string("item-") + std::to_string(i));
	bool passed = (a.size() == 50);
	for (int i = 0; i < 50 && passed; ++i)
		if (a[i] != ("item-" + std::to_string(i))) passed = false;

	// Force growth path with a non-trivial T to exercise the move-construct loop.
	for (int i = 50; i < 500; ++i)
		a.emplace_back(std::string("item-") + std::to_string(i));
	for (int i = 0; i < 500 && passed; ++i)
		if (a[i] != ("item-" + std::to_string(i))) passed = false;

	TestRunner::EndTest(passed);
}

static void TestArrayPopBack()
{
	TestRunner::StartTest("Array: pop_back shrinks size, keeps capacity");

	Array<int> a;
	for (int i = 0; i < 10; ++i) a.push_back(i);
	const size_t cap = a.capacity();
	for (int i = 0; i < 5; ++i) a.pop_back();
	bool passed = (a.size() == 5) && (a.capacity() == cap);
	for (int i = 0; i < 5 && passed; ++i)
		if (a[i] != i) passed = false;
	TestRunner::EndTest(passed);
}

static void TestArrayResize()
{
	TestRunner::StartTest("Array: resize up + down with fill value");

	Array<int> a;
	a.resize(50, 7);
	bool passed = (a.size() == 50);
	for (int i = 0; i < 50 && passed; ++i)
		if (a[i] != 7) passed = false;

	a.resize(10);
	passed = passed && (a.size() == 10);
	for (int i = 0; i < 10 && passed; ++i)
		if (a[i] != 7) passed = false;

	a.resize(20, 99);
	passed = passed && (a.size() == 20);
	for (int i = 0; i < 10 && passed; ++i)
		if (a[i] != 7) passed = false;
	for (int i = 10; i < 20 && passed; ++i)
		if (a[i] != 99) passed = false;

	TestRunner::EndTest(passed);
}

static void TestArrayShrinkToFit()
{
	TestRunner::StartTest("Array: shrink_to_fit reduces capacity to size");

	Array<int> a;
	a.reserve(1000);
	for (int i = 0; i < 50; ++i) a.push_back(i);
	a.shrink_to_fit();
	bool passed = (a.size() == 50) && (a.capacity() == 50);
	for (int i = 0; i < 50 && passed; ++i)
		if (a[i] != i) passed = false;
	TestRunner::EndTest(passed);
}

static void TestArraySwap()
{
	TestRunner::StartTest("Array: swap exchanges contents");

	Array<int> a;
	for (int i = 0; i < 5; ++i) a.push_back(i);
	Array<int> b;
	for (int i = 100; i < 110; ++i) b.push_back(i);

	a.swap(b);
	bool passed = (a.size() == 10) && (b.size() == 5);
	for (int i = 0; i < 10 && passed; ++i)
		if (a[i] != 100 + i) passed = false;
	for (int i = 0; i < 5 && passed; ++i)
		if (b[i] != i) passed = false;
	TestRunner::EndTest(passed);
}

static void TestArrayMoveCtor()
{
	TestRunner::StartTest("Array: move ctor leaves source empty + transfers buffer");

	Array<int> a;
	for (int i = 0; i < 10; ++i) a.push_back(i);
	const size_t cap = a.capacity();
	Array<int> b(std::move(a));
	bool passed = (b.size() == 10) && (b.capacity() == cap) && (a.size() == 0) && (a.capacity() == 0);
	for (int i = 0; i < 10 && passed; ++i)
		if (b[i] != i) passed = false;
	TestRunner::EndTest(passed);
}

void RunArrayUnitTests()
{
	TestRunner::StartTestSuite("Array Unit Tests");

	TestArrayBasicOperations();
	TestArrayIterators();
	TestArrayFixedSizeDesign();
	TestArrayGrowFromEmpty();
	TestArrayGrowPastReserve();
	TestArrayNonTrivialT();
	TestArrayPopBack();
	TestArrayResize();
	TestArrayShrinkToFit();
	TestArraySwap();
	TestArrayMoveCtor();

	TestRunner::EndTestSuite();
}
