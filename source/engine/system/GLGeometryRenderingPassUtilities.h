#pragma once
#include "../common/InnoType.h"

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeGeometryPass();

	void updateGeometryPass();

	bool resizeGeometryPass();

	bool reloadOpaquePassShaders();

	bool reloadTransparentPassShaders();

	bool reloadTerrainPassShaders();
}
