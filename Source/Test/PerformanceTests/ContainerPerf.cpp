#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/Array.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void TestArrayPerformance()
{
	TestRunner::StartTest("Array vs STL Vector Performance");

	const size_t l_TestCount = TestConfig::MediumDataSize;

	// Test custom Array implementation
	auto l_CustomTime = TestTimer::MeasureFunction([&]()
	{
		Array<float> l_Array;
		l_Array.reserve(l_TestCount);

		for (size_t i = 0; i < l_TestCount; i++)
		{
			l_Array.emplace_back(static_cast<float>(i));
		}

		auto l_ArrayCopy = l_Array;

		Array<float> l_Array2;
		l_Array2.reserve(l_TestCount);
		for (size_t i = 0; i < l_TestCount; i++)
		{
			l_Array2.emplace_back(static_cast<float>(i));
		}
	});

	// Test STL vector implementation
	auto l_STLTime = TestTimer::MeasureFunction([&]()
	{
		std::vector<float> l_STLArray;
		l_STLArray.reserve(l_TestCount);
		for (size_t i = 0; i < l_TestCount; i++)
		{
			l_STLArray.emplace_back(static_cast<float>(i));
		}
		
		auto l_STLArrayCopy = l_STLArray;
		
		std::vector<float> l_STLArray2;
		l_STLArray2.reserve(l_TestCount);
		for (size_t i = 0; i < l_TestCount; i++)
		{
			l_STLArray2.emplace_back(static_cast<float>(i));
		}
	});

	auto l_SpeedRatio = l_CustomTime / l_STLTime;
	Log(Success, "Custom Array vs STL vector speed ratio: ", l_SpeedRatio);
	Log(Success, "Custom time: ", l_CustomTime, "ms, STL time: ", l_STLTime, "ms");

	TestRunner::EndTest(true);
}

void RunContainerPerformanceTests()
{
	TestRunner::StartTestSuite("Container Performance Tests");
	
	TestArrayPerformance();
	
	TestRunner::EndTestSuite();
}
