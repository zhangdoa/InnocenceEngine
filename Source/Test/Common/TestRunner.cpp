#include "TestRunner.h"
#include "TestTimer.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

// Static member definitions
size_t TestRunner::m_TotalTests = 0;
size_t TestRunner::m_PassedTests = 0;
size_t TestRunner::m_FailedTests = 0;
std::string TestRunner::m_CurrentSuite;
std::string TestRunner::m_CurrentTest;
TestTimer* TestRunner::m_Timer = nullptr;

void TestRunner::StartTestSuite(const char* suiteName)
{
	m_CurrentSuite = suiteName;
	Log(Success, "=== Starting Test Suite: ", suiteName, " ===");
}

void TestRunner::EndTestSuite()
{
	Log(Success, "=== Completed Test Suite: ", m_CurrentSuite.c_str(), " ===");
	m_CurrentSuite.clear();
}

void TestRunner::StartTest(const char* testName)
{
	m_CurrentTest = testName;
	m_TotalTests++;
	
	if (!m_Timer)
	{
		m_Timer = new TestTimer();
	}
	
	m_Timer->Start();
	Log(Verbose, "Running test: ", testName);
}

void TestRunner::EndTest(bool passed)
{
	if (m_Timer)
	{
		m_Timer->Stop();
	}
	
	if (passed)
	{
		m_PassedTests++;
		Log(Success, "✓ ", m_CurrentTest.c_str(), " - ", 
			m_Timer ? m_Timer->GetDurationMs() : 0.0, "ms");
	}
	else
	{
		m_FailedTests++;
		Log(Error, "✗ ", m_CurrentTest.c_str(), " - FAILED");
	}
	m_CurrentTest.clear();
}

void TestRunner::ReportResults()
{
	Log(Success, "\n=== TEST RESULTS ===");
	Log(Success, "Total Tests: ", m_TotalTests);
	Log(Success, "Passed: ", m_PassedTests);
	if (m_FailedTests > 0)
	{
		Log(Error, "Failed: ", m_FailedTests);
	}
	else
	{
		Log(Success, "Failed: ", m_FailedTests);
	}
	double l_SuccessRate = (double)m_PassedTests / (double)m_TotalTests * 100.0;
	Log(Success, "Success Rate: ", l_SuccessRate, "%");
	
	// Cleanup
	if (m_Timer)
	{
		delete m_Timer;
		m_Timer = nullptr;
	}
}

// Forward declarations for test suites (to be implemented)
extern void RunObjectPoolUnitTests();
extern void RunArrayUnitTests();
extern void RunAtomicUnitTests();
extern void RunRingBufferUnitTests();

extern void RunStringConversionPerformanceTests();
extern void RunContainerPerformanceTests();
extern void RunMemoryPerformanceTests();

extern void RunConcurrencyStressTests();
extern void RunMemoryStressTests();

void TestRunner::RunUnitTests()
{
	Log(Success, "\n========== UNIT TESTS ==========");
	
	RunObjectPoolUnitTests();
	RunArrayUnitTests();
	RunAtomicUnitTests();
	RunRingBufferUnitTests();
	
	Log(Success, "========== UNIT TESTS COMPLETE ==========\n");
}

void TestRunner::RunPerformanceTests()
{
	Log(Success, "\n========== PERFORMANCE TESTS ==========");
	
	RunStringConversionPerformanceTests();
	RunContainerPerformanceTests();
	RunMemoryPerformanceTests();
	
	Log(Success, "========== PERFORMANCE TESTS COMPLETE ==========\n");
}

void TestRunner::RunStressTests()
{
	Log(Success, "\n========== STRESS TESTS ==========");
	
	RunConcurrencyStressTests();
	RunMemoryStressTests();
	
	Log(Success, "========== STRESS TESTS COMPLETE ==========\n");
}

void TestRunner::RunAllTests()
{
	Log(Success, "\nStarting InnocenceEngine Test Suite...\n");
	
	RunUnitTests();
	RunPerformanceTests();
	RunStressTests();
	
	ReportResults();
}
