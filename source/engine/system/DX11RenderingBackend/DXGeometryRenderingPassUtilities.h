#pragma once
#include "../../common/InnoType.h"

INNO_PRIVATE_SCOPE DXGeometryRenderingPassUtilities
{
	void initialize();

	void update();

	bool resize();

	bool reloadOpaquePassShaders();

	bool reloadTransparentPassShaders();

	bool reloadTerrainPassShaders();
}
