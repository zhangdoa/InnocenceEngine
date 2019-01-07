#pragma once
#include "../common/InnoType.h"

INNO_PRIVATE_SCOPE GLFinalRenderingPassUtilities
{
	void initialize();

	void update();

	bool resize();

	bool reloadFinalPassShaders();
}