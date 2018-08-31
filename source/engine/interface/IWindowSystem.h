#pragma once
#include "ISystem.h"
#include "../common/InnoMath.h"

class IWindowSystem : public ISystem
{
public:
	virtual ~IWindowSystem() {};
	virtual vec4 calcMousePositionInWorldSpace() = 0;
	virtual void swapBuffer() = 0;
};