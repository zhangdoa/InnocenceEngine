#include "../Common/TestRunner.h"
#include "../Common/TestTimer.h"
#include "../Common/TestData.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void TestStringConversionPerformance()
{
	TestRunner::StartTest("String Conversion Performance");

	const size_t l_TestCount = TestConfig::PerformanceIterations;
	int64_t l_Int64 = std::numeric_limits<int64_t>::max();
	int32_t l_Int32 = std::numeric_limits<int32_t>::max();

	// Test std::to_string with int32
	auto l_Int32Time = TestTimer::MeasurePerformance(l_TestCount, [&]()
	{
		auto l_StringFromInt32 = std::to_string(l_Int32);
	});

	// Test std::to_string with int64
	auto l_Int64Time = TestTimer::MeasurePerformance(l_TestCount, [&]()
	{
		auto l_StringFromInt64 = std::to_string(l_Int64);
	});

	auto l_SpeedRatio = l_Int64Time / l_Int32Time;
	Log(Success, "Int64 vs Int32 string conversion speed ratio: ", l_SpeedRatio);
	Log(Success, "Int32 time: ", l_Int32Time, "ms, Int64 time: ", l_Int64Time, "ms");

	TestRunner::EndTest(true);
}

void RunStringConversionPerformanceTests()
{
	TestRunner::StartTestSuite("String Conversion Performance Tests");
	
	TestStringConversionPerformance();
	
	TestRunner::EndTestSuite();
}
