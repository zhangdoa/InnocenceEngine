#pragma once
#include "../Common/STL14.h"

namespace Inno
{
	class InnoRandomizer
	{
	public:
		static bool Setup();

		static uint64_t GenerateUUID();
	};
}