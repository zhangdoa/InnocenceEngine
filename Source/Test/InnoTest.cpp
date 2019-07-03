#include "../Engine/Common/InnoContainer.h"
#include "../Engine/Common/InnoMath.h"
#include "../Engine/Core/InnoTimer.h"
#include "../Engine/Core/InnoLogger.h"
#include "../Engine/Core/InnoMemory.h"

void TestIToA(size_t testCaseCount)
{
	int64_t int64 = std::numeric_limits<int64_t>::max();
	int32_t int32 = std::numeric_limits<int32_t>::max();

	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = ToString(int32);
		auto l_StringFromInt64 = ToString(int64);
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_StringFromInt32 = std::to_string(int32);
		auto l_StringFromInt64 = std::to_string(int64);
	}

	auto l_Timestamp2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	InnoLogger::Log(LogLevel::Success, "Custom VS STL IToA speed ratio is ", l_SpeedRatio);
}

void TestInnoArray(size_t testCaseCount)
{
	auto l_StartTime = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	InnoArray<float> l_Array;
	l_Array.reserve(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_Array[i] = (float)i;
	}
	auto l_ArrayCopy = l_Array;

	InnoArray<float> l_Array2;
	l_Array2.reserve(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_Array2.emplace_back((float)i);
	}

	auto l_Timestamp1 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	std::vector<float> l_STLArray;
	l_STLArray.resize(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_STLArray[i] = (float)i;
	}
	auto l_STLArrayCopy = l_STLArray;
	std::vector<float> l_STLArray2;
	l_STLArray2.resize(testCaseCount);
	for (size_t i = 0; i < testCaseCount; i++)
	{
		l_STLArray2.emplace_back((float)i);
	}

	auto l_Timestamp2 = InnoTimer::GetCurrentTimeFromEpoch(TimeUnit::Microsecond);

	auto l_SpeedRatio = double(l_Timestamp1 - l_StartTime) / double(l_Timestamp2 - l_Timestamp1);

	InnoLogger::Log(LogLevel::Success, "Custom VS STL array container speed ratio is ", l_SpeedRatio);
}

void TestInnoMemory(size_t testCaseCount)
{
	std::default_random_engine l_generator;
	std::uniform_int_distribution<unsigned int> l_randomDelta(1, 16384);

	for (size_t i = 0; i < testCaseCount; i++)
	{
		auto l_allocateSize = l_randomDelta(l_generator) * l_randomDelta(l_generator);
		auto l_ptr = InnoMemory::Allocate(l_allocateSize);
		InnoLogger::Log(LogLevel::Success, "Memory allocated at ", l_ptr, " for ", l_allocateSize, "Byte(s)");
		InnoMemory::Deallocate(l_ptr);
		InnoLogger::Log(LogLevel::Success, "Memory deallocated at ", l_ptr);
	}
}

int main(int argc, char *argv[])
{
	TestIToA(1024);
	TestInnoArray(8192);
	TestInnoMemory(128);

	while (1);
	return 0;
}