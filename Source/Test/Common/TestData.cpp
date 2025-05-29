#include "TestData.h"

using namespace Inno;

std::vector<int32_t> TestDataGenerator::GenerateIntSequence(size_t count)
{
	std::vector<int32_t> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back(static_cast<int32_t>(i));
	}
	return l_Data;
}

std::vector<float> TestDataGenerator::GenerateFloatSequence(size_t count)
{
	std::vector<float> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back(static_cast<float>(i));
	}
	return l_Data;
}

std::vector<std::string> TestDataGenerator::GenerateStringData(size_t count)
{
	std::vector<std::string> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back("TestString_" + std::to_string(i));
	}
	return l_Data;
}
