#include "Common/TestRunner.h"
#include "../Engine/Engine.h"
#include "../Engine/Common/LogService.h"

using namespace Inno;

int main(int argc, char* argv[])
{
	// Create engine instance for headless testing
	std::unique_ptr<Engine> l_pEngine = std::make_unique<Engine>();
	
	// Setup engine for headless operation (no window handles needed)
	if (!l_pEngine->Setup(nullptr, nullptr, "headless"))
	{
		Log(Error, "Failed to setup engine for testing");
		return 1;
	}
	
	// Initialize engine for tests that require it
	if (!l_pEngine->Initialize())
	{
		Log(Error, "Failed to initialize engine for testing");
		return 1;
	}
	
	if (argc > 1)
	{
		std::string l_TestType = argv[1];
		
		if (l_TestType == "unit" || l_TestType == "-u")
		{
			TestRunner::RunUnitTests();
		}
		else if (l_TestType == "performance" || l_TestType == "-p")
		{
			TestRunner::RunPerformanceTests();
		}
		else if (l_TestType == "stress" || l_TestType == "-s")
		{
			TestRunner::RunStressTests();
		}
		else if (l_TestType == "help" || l_TestType == "-h")
		{
			Log(Success, "InnocenceEngine Test Runner");
			Log(Success, "Usage: Test.exe [test_type]");
			Log(Success, "");
			Log(Success, "Test Types:");
			Log(Success, "  unit, -u        Run unit tests only");
			Log(Success, "  performance, -p Run performance tests only");
			Log(Success, "  stress, -s      Run stress tests only");
			Log(Success, "  help, -h        Show this help message");
			Log(Success, "");
			Log(Success, "If no arguments provided, all tests will run.");
			l_pEngine->Terminate();
			return 0;
		}
		else
		{
			Log(Error, "Unknown test type: ", l_TestType.c_str());
			Log(Success, "Use 'Test.exe help' for usage information.");
			l_pEngine->Terminate();
			return 1;
		}
	}
	else
	{
		// Run all tests by default
		TestRunner::RunAllTests();
	}

	// Terminate engine
	l_pEngine->Terminate();
	return 0;
}
