#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/DX11RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX11LightCullingPass
{
	bool initialize();

	bool update();

	bool resize();

	bool reloadShaders();

	DX11RenderPassComponent* getDX11RPC();

	DX11TextureDataComponent* getLightGrid();

	DX11TextureDataComponent* getHeatMap();
}