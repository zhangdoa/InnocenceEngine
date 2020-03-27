#include "InnoRandomizer.h"

namespace InnoRandomizerNS
{
	std::random_device rd;
	std::mt19937_64 e2(rd());
	std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
};

using namespace InnoRandomizerNS;

bool InnoRandomizer::Setup()
{
	return true;
}

uint64_t InnoRandomizer::GenerateUUID()
{
	return dist(e2);
}