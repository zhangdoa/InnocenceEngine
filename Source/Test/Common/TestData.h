#pragma once
#include "../../Engine/Common/STL14.h"

namespace Inno
{
	// Common test data structures and utilities
	struct TestConfig
	{
		static constexpr size_t SmallDataSize = 1024;
		static constexpr size_t MediumDataSize = 8192;
		static constexpr size_t LargeDataSize = 65536;
		static constexpr size_t StressTestSize = 262144;

		static constexpr size_t ConcurrencyTestThreads = 128;
		static constexpr size_t PerformanceIterations = 10000;
	};

	// Test data for memory allocations
	struct TestStruct
	{
		uint32_t m_Data[256];
	};

	// Utility functions for test data generation (no engine APIs)
	class TestDataGenerator
	{
	public:
		static std::vector<int32_t> GenerateIntSequence(size_t count);
		static std::vector<float> GenerateFloatSequence(size_t count);
		static std::vector<std::string> GenerateStringData(size_t count);
	};
}
