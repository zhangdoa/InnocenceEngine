#pragma once
#include "../../Engine/Common/STL14.h"

namespace Inno
{
	class TestTimer;

	class TestRunner
	{
	public:
		// Static interface - implementations in .cpp file (v1.1 standard)
		static void RunAllTests();
		static void RunUnitTests();
		static void RunPerformanceTests();
		static void RunStressTests();

		static void StartTestSuite(const char* suiteName);
		static void EndTestSuite();
		static void StartTest(const char* testName);
		static void EndTest(bool passed = true);

		static void ReportResults();

	private:
		// Static member variables
		static size_t m_TotalTests;
		static size_t m_PassedTests;
		static size_t m_FailedTests;
		static std::string m_CurrentSuite;
		static std::string m_CurrentTest;
		static TestTimer* m_Timer;
	};
}
