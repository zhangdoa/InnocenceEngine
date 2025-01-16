#include "Randomizer.h"

using namespace Inno;
namespace Inno
{
	namespace RandomizerNS
	{
		std::random_device rd;
		std::mt19937_64 e2(rd());
		std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
	}
}

using namespace RandomizerNS;

bool Randomizer::Setup()
{
	return true;
}

uint64_t Randomizer::GenerateUUID()
{
	return dist(e2);
}