#include "InnoMath.h"

__declspec(dllexport) EntityID InnoMath::createEntityID()
{
	return std::rand();
}
