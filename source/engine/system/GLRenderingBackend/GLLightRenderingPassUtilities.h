#pragma once
#include "../../common/InnoType.h"

INNO_PRIVATE_SCOPE GLLightRenderingPassUtilities
{
	void initialize();

	void update();

	bool resize();

	bool reloadLightPassShaders();
}