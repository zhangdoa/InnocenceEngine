#pragma once
#include "../../common/InnoType.h"

INNO_PRIVATE_SCOPE GLFinalRenderPass
{
	void initialize();

	void update();

	bool resize();

	bool reloadFinalPassShaders();
}