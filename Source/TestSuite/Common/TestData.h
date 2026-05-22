#pragma once
#include "../../Engine/Common/Array.h"
#include "../../Engine/Common/STL14.h"

namespace Inno
{
	struct TestConfig
	{
		static constexpr size_t SmallDataSize = 1024;
		static constexpr size_t MediumDataSize = 8192;
		static constexpr size_t LargeDataSize = 65536;
		static constexpr size_t StressTestSize = 262144;

		static constexpr size_t ConcurrencyTestThreads = 128;
		static constexpr size_t PerformanceIterations = 10000;
	};

	struct TestStruct
	{
		uint32_t m_Data[256];
	};

	class TestDataGenerator
	{
	public:
		static Inno::Array<int32_t> GenerateIntSequence(size_t count);
		static Inno::Array<float> GenerateFloatSequence(size_t count);
		static Inno::Array<std::string> GenerateStringData(size_t count);
	};
}
