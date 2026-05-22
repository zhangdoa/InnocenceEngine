#include "TestData.h"
#include "../../Engine/Common/Array.h"

using namespace Inno;

Inno::Array<int32_t> TestDataGenerator::GenerateIntSequence(size_t count)
{
	Inno::Array<int32_t> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back(static_cast<int32_t>(i));
	}
	return l_Data;
}

Inno::Array<float> TestDataGenerator::GenerateFloatSequence(size_t count)
{
	Inno::Array<float> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back(static_cast<float>(i));
	}
	return l_Data;
}

Inno::Array<std::string> TestDataGenerator::GenerateStringData(size_t count)
{
	Inno::Array<std::string> l_Data;
	l_Data.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		l_Data.emplace_back("TestString_" + std::to_string(i));
	}
	return l_Data;
}
