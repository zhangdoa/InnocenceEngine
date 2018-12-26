#pragma once
#include "../common/InnoType.h"

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeLightPass();

	void updateLightPass();

	bool resizeLightPass();

	bool reloadLightPassShaders();
}