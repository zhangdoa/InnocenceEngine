#pragma once
#include "../Common/STL14.h"

namespace Inno
{
	class Randomizer
	{
	public:
		static bool Setup();

		static uint64_t GenerateUUID();
	};
}