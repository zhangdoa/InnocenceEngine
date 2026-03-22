#include "../Common/TestRunner.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Array.h"

using namespace Inno;

void TestArrayBasicOperations()
{
	TestRunner::StartTest("Array Basic Operations");

	Array<int32_t> l_Array;
	bool l_TestPassed = true;

	// Test basic operations
	l_Array.reserve(100);
	for (size_t i = 0; i < 50; i++)
	{
		l_Array.emplace_back(static_cast<int32_t>(i));
	}

	// Test size and capacity
	if (l_Array.size() != 50)
	{
		l_TestPassed = false;
	}

	// Test element access
	for (size_t i = 0; i < 50; i++)
	{
		if (l_Array[i] != static_cast<int32_t>(i))
		{
			l_TestPassed = false;
			break;
		}
	}

	// Test copy construction
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

	// Reserve capacity before adding elements
	l_Array.reserve(25);

	// Add test data
	for (size_t i = 0; i < 20; i++)
	{
		l_Array.emplace_back(static_cast<float>(i));
	}

	// Test range-based for loop
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

	// Test empty array behavior
	Array<int> l_EmptyArray;
	if (l_EmptyArray.size() != 0 || l_EmptyArray.capacity() != 0 || l_EmptyArray.is_initialized())
	{
		l_TestPassed = false;
	}

	// Test safe empty iteration
	size_t l_IterCount = 0;
	for (auto& value : l_EmptyArray)
	{
		l_IterCount++; // Should never execute
	}
	if (l_IterCount != 0)
	{
		l_TestPassed = false;
	}

	// Test reserve and capacity lock
	Array<int> l_Array;
	l_Array.reserve(50);
	if (!l_Array.is_initialized() || l_Array.capacity() != 50 || l_Array.size() != 0)
	{
		l_TestPassed = false;
	}

	// Test capacity cannot be changed after initialization
	// Note: This would assert in debug mode, so we only test successful operations

	// Test normal operations after reserve
	for (size_t i = 0; i < 30; i++)
	{
		l_Array.emplace_back(static_cast<int>(i * 2));
	}

	if (l_Array.size() != 30 || l_Array.capacity() != 50)
	{
		l_TestPassed = false;
	}

	// Test element access
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

void RunArrayUnitTests()
{
	TestRunner::StartTestSuite("Array Unit Tests");
	
	TestArrayBasicOperations();
	TestArrayIterators();
	TestArrayFixedSizeDesign();
	
	TestRunner::EndTestSuite();
}
