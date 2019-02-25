#pragma once
#include "../common/InnoType.h"

INNO_PRIVATE_SCOPE DXFinalRenderingPassUtilities
{
	void initialize();

	void update();

	bool resize();

	bool reloadFinalPassShaders();
}